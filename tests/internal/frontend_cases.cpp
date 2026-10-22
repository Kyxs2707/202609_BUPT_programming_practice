#include "ast/factory.hpp"
#include "frontend/driver.hpp"
#include "frontend/flex_allocations.hpp"
#include "semantic/checker.hpp"
#include "source/source.hpp"
#include "test_hooks.hpp"
#include "test_support.hpp"
#include <type_traits>
using namespace myb;
static_assert(std::has_virtual_destructor_v<Node>);
static_assert(std::is_abstract_v<AstVisitor>);
MYB_TEST(ast_contract) {
    EmergencyFailure first;
    AstFactory f(first);
    SourceSpan span{0, 0, 1};
    std::unique_ptr<Expr> e = f.make<IntExpr>(span, std::string("1"));
    REQUIRE(e->height() == 1);
    for (int i = 1; i < 256; ++i)
        e = f.make<GroupExpr>(span, std::move(e));
    REQUIRE(e->height() == 256);
    try {
        e = f.make<GroupExpr>(span, std::move(e));
        REQUIRE(false);
    } catch (const HostFailure &h) {
        REQUIRE(h.header().reason == ReasonCode::Capacity);
    }
    REQUIRE(e->height() == 256);
}
MYB_TEST(frontend_smoke) {
    EmergencyFailure first;
    SourceManager sources(first);
    auto id = sources.add("smoke", "print(1 + 2);");
    Driver d(sources, id);
    auto ast = d.parse();
    REQUIRE(ast->items().size() == 1);
    auto &statement = dynamic_cast<const ExprStmt &>(ast->items()[0]);
    auto &call = dynamic_cast<const CallExpr &>(statement.expr());
    REQUIRE(call.name() == "print");
    REQUIRE(call.name_span().begin == 0);
    REQUIRE(call.name_span().end == 5);
    REQUIRE(call.args().size() == 1);
    auto &add = dynamic_cast<const BinaryExpr &>(call.args()[0]);
    REQUIRE(std::get<BinaryOp>(add.op()) == BinaryOp::Add);
    REQUIRE(add.operator_span().begin == 8);
    Checker checker(first);
    auto checked = checker.check(*ast);
    REQUIRE(checked.literals.size() == 2);
    auto empty = sources.add("empty", "");
    Driver empty_driver(sources, empty);
    REQUIRE(empty_driver.parse()->items().empty());
    auto bad = sources.add("bad", std::string("@\0x", 3));
    try {
        Driver bad_driver(sources, bad);
        bad_driver.parse();
        REQUIRE(false);
    } catch (const FrontendFailure &) {
    }
    Driver again(sources, id);
    REQUIRE(again.parse()->items().size() == 1);
    TestHooks::fail_next(FaultPoint::FlexAllocation);
    try {
        Driver injected(sources, id);
        injected.parse();
        REQUIRE(false);
    } catch (const HostFailure &h) {
        REQUIRE(h.header().domain == FailureDomain::Resource);
    }
    REQUIRE(FlexAllocationLedger::live_total() == 0);
    TestHooks::clear_fault();
}

MYB_TEST(frontend_edges) {
    auto parse = [](SourceManager &s, std::string input) {
        auto id = s.add("edges", std::move(input));
        Driver d(s, id);
        return d.parse();
    };
    EmergencyFailure first;
    SourceManager sources(first);
    Checker checker(first);
    for (auto input : {"-9223372036854775808;", "(-9223372036854775808);",
                       "- 0009223372036854775808;", "-(-9223372036854775808);"}) {
        auto tree = parse(sources, input);
        auto c = checker.check(*tree);
        REQUIRE(!c.literals.empty());
    }
    for (auto input : {"9223372036854775808;", "-(9223372036854775808);", "9223372036854775809;"}) {
        try {
            auto tree = parse(sources, input);
            checker.check(*tree);
            REQUIRE(false);
        } catch (const FrontendFailure &f) {
            REQUIRE(f.header().reason == ReasonCode::Range);
        }
    }
    for (auto input : {"print(1,);", "1e;", "1e+;", "1.;", ".5;", "12abc;", "0x12;", "1++;", "1--;",
                       "1+=2;", ";", "\"bad\\q\";", "\"bad\\u{D800}\";", "\"bad\\u{110000}\";",
                       "\"bad\\u{}\";", "\"bad\\0\";", "\"bad\n\";", "\"bad"}) {
        try {
            parse(sources, input);
            REQUIRE(false);
        } catch (const FrontendFailure &) {
        }
    }
    auto unicode = parse(sources, "\xEF\xBB\xBF\r\n# text\r'\\u{0}\\u{1F600}';");
    auto &literal = dynamic_cast<const StringExpr &>(
        dynamic_cast<const ExprStmt &>(unicode->items()[0]).expr());
    REQUIRE(literal.value() == std::string("\0\xF0\x9F\x98\x80", 5));
    REQUIRE(literal.span().begin == 12);
    try {
        parse(sources, std::string("1;\0print(1);", 12));
        REQUIRE(false);
    } catch (const FrontendFailure &f) {
        REQUIRE(f.header().span->begin == 2);
        REQUIRE(f.header().span->end == 3);
    }
    try {
        parse(sources, "\xF0\x9F\x98\x80;");
        REQUIRE(false);
    } catch (const FrontendFailure &f) {
        REQUIRE(f.header().span->begin == 0);
        REQUIRE(f.header().span->end == 4);
    }
    REQUIRE(FlexAllocationLedger::live_total() == 0);
}

MYB_TEST(ast_dispatch_and_cleanup) {
    struct Counter final : AstVisitor {
        int count{};
#define COUNT(T)                                                                                   \
    void visit(const T &) override {                                                               \
        ++count;                                                                                   \
    }
        COUNT(AstProgram)
        COUNT(FunctionDecl) COUNT(StructDecl) COUNT(IntExpr) COUNT(FloatExpr) COUNT(StringExpr)
            COUNT(BoolExpr) COUNT(NullExpr) COUNT(NameExpr) COUNT(GroupExpr) COUNT(UnaryExpr)
                COUNT(BinaryExpr) COUNT(LogicalExpr) COUNT(CallExpr) COUNT(ArrayExpr)
                    COUNT(ConstructExpr) COUNT(FieldExpr) COUNT(IndexExpr) COUNT(SliceExpr)
                        COUNT(LetStmt) COUNT(AssignStmt) COUNT(ExprStmt) COUNT(BlockStmt)
                            COUNT(IfStmt) COUNT(WhileStmt) COUNT(ForStmt) COUNT(ReturnStmt)
                                COUNT(BreakStmt) COUNT(ContinueStmt)
#undef COUNT
    };
    struct DestructionProbe final : Expr {
        int &destroyed;
        explicit DestructionProbe(int &d) : Expr({0, 0, 0}, 1), destroyed(d) {}
        ~DestructionProbe() noexcept override {
            ++destroyed;
        }
        void accept(AstVisitor &) const override {}
    };
    EmergencyFailure first;
    AstFactory f(first);
    SourceSpan s{0, 0, 0};
    Counter visitor;
    auto group = f.make<GroupExpr>(s, f.make<IntExpr>(s, std::string("1")));
    group->accept(visitor);
    REQUIRE(visitor.count == 1);
    int destroyed = 0;
    {
        std::unique_ptr<Node> root =
            f.make<GroupExpr>(s, std::make_unique<DestructionProbe>(destroyed));
    }
    REQUIRE(destroyed == 1);
    std::unique_ptr<Expr> expression = f.make<IntExpr>(s, std::string("1"));
    for (int i = 1; i < 254; ++i)
        expression = f.make<GroupExpr>(s, std::move(expression));
    std::vector<std::unique_ptr<TopItem>> items;
    items.push_back(f.make<ExprStmt>(s, std::move(expression)));
    auto root = f.make<AstProgram>(s, std::move(items));
    REQUIRE(root->height() == 256);
    auto *one = static_cast<const ReadOnlyNodes<TopItem> *>(&root->items());
    static_assert(std::is_same_v<decltype((*one)[0]), const TopItem &>);
}
MYB_TEST(flex_ledger_failure) {
    EmergencyFailure first;
    REQUIRE(FlexAllocationLedger::live_total() == 0);
    {
        FlexAllocationLedger ledger(first);
        auto *allocation = static_cast<char *>(ledger.allocate(16));
        allocation[0] = 'x';
        REQUIRE(FlexAllocationLedger::live_total() == 1);
        TestHooks::fail_next(FaultPoint::FlexAllocation);
        try {
            ledger.reallocate(allocation, 32);
            REQUIRE(false);
        } catch (const HostFailure &h) {
            REQUIRE(h.header().domain == FailureDomain::Resource);
        }
        REQUIRE(allocation[0] == 'x');
        REQUIRE(FlexAllocationLedger::live_total() == 1);
        TestHooks::clear_fault();
        auto *larger = static_cast<char *>(ledger.reallocate(allocation, 32));
        REQUIRE(larger[0] == 'x');
        REQUIRE(FlexAllocationLedger::live_total() == 1);
        ledger.release(larger);
    }
    REQUIRE(FlexAllocationLedger::live_total() == 0);
}
#include "frontend/scanner.hpp"
int yylex_init_extra(myb::ScannerContext *, yyscan_t *);
int yylex_destroy(yyscan_t);
yy_buffer_state *yy_scan_buffer(char *, std::size_t, yyscan_t);
MYB_TEST(flex_fatal_classification) {
    for (auto message : {"out of dynamic memory in scanner", "scanner buffer overflow",
                         "fatal scanner internal error"}) {
        EmergencyFailure first;
        ScannerContext context(0, first);
        yyscan_t handle{};
        REQUIRE(yylex_init_extra(&context, &handle) == 0);
        try {
            flex_fatal(message, handle);
            REQUIRE(false);
        } catch (const HostFailure &f) {
            auto text = std::string_view(message);
            REQUIRE(f.header().domain == (text.find("internal") != text.npos
                                              ? FailureDomain::Internal
                                              : FailureDomain::Resource));
            REQUIRE(f.header().reason == (text.find("internal") != text.npos ? ReasonCode::Invariant
                                          : text.find("overflow") != text.npos
                                              ? ReasonCode::Capacity
                                              : ReasonCode::Allocation));
        }
        REQUIRE(yylex_destroy(handle) == 0);
        REQUIRE(FlexAllocationLedger::live_total() == 0);
    }
    EmergencyFailure first;
    ScannerContext context(0, first);
    yyscan_t handle{};
    REQUIRE(yylex_init_extra(&context, &handle) == 0);
    char bytes[] = {'1', ';', 0, 0};
    TestHooks::fail_next(FaultPoint::FlexAllocation);
    try {
        yy_scan_buffer(bytes, sizeof bytes, handle);
        REQUIRE(false);
    } catch (const HostFailure &) {
    }
    REQUIRE(yylex_destroy(handle) == 0);
    REQUIRE(FlexAllocationLedger::live_total() == 0);
    TestHooks::clear_fault();
}
