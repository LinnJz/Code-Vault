#pragma once

#include <cassert>
#include <cstdio>
#include <string>

#include "../../stdex/utility/renum.hpp"

// ============ 通用版测试：tag存储，值/引用/cv成员全覆盖 ============

struct Widget {          // 非平凡析构
  int tag = 0;
  ~Widget() { tag = -1; }
};

struct All {             // POD=int，非POD=std::string
  int m0;
  volatile int m1;
  const int m2;
  volatile const int m3;
  int& m4;
  int&& m5;
  const int& m6;
  int* m7;
  std::string m8;
  volatile std::string m9;
  const std::string m10;
  volatile const std::string m11;
  std::string& m12;
  std::string&& m13;
  const std::string& m14;
  std::string* m15;
};

struct AllW {            // 自定义非平凡类型
  Widget w0;
  Widget& w1;
  Widget* w2;
};

template <class T>
struct R : std::__storage_base<T> {};

// ============ niche测试：引用niche / bool niche / 类型索引 / get_if ============

// ---- niche开关（仿map::is_transparent）：using is_niche = void; 显式开启 ----
// 不写开关的类型即使满足条件也走tag版（默认关闭）
struct Ref2   { int& a; std::string& b; using is_niche = void; };            // 全引用+开关：可niche
struct Ref1   { int& a; using is_niche = void; };                            // 单引用+开关：可niche
struct Ref3   { int& a; std::string& b; double& c; using is_niche = void; }; // 3引用：2位对齐位足够
struct NoNiche { int& a; char& b; using is_niche = void; };                  // char对齐1，无位可借 → tag回退
struct Mixed  { int x; std::string& s; };                                       // 无开关+含值成员 → tag回退
struct Ref2NoOpt { int& a; std::string& b; };                                   // 满足条件但无开关 → tag回退

// ---- bool niche（Rust语义）：恰好1个bool + 其余空类 ----
struct Bool1  { bool a; using is_niche = void; };                              // 单bool+开关：可niche
struct Bool2  { bool a; struct None {} none; using is_niche = void; };         // bool+空类+开关：可niche
struct Bool3  { bool a; struct None {} none; struct Empty {} e; using is_niche = void; }; // bool+2空类
struct BoolFail { bool a; bool b; using is_niche = void; };                    // 双bool：值域重叠 → tag回退
struct Bool1NoOpt { bool a; };                                                    // 满足条件但无开关 → tag回退
struct Amb     { int a; int b; };                                                 // 同型成员：按类型歧义 → 哨兵

// ---- 按类型索引查找（__index_of，精确匹配含cvref） ----
static_assert(std::__storage_meta<Ref2>::__index_of<int&> == 0);          // int&成员
static_assert(std::__storage_meta<Ref2>::__index_of<std::string&> == 1);  // string&成员
static_assert(std::__storage_meta<Ref2>::__index_of<int> == 2);           // 无精确匹配 → 哨兵
static_assert(std::__storage_meta<Ref2>::__index_of<const int&> == 2);    // int&成员 ≠ const int&
static_assert(std::__storage_meta<Mixed>::__index_of<int> == 0);
static_assert(std::__storage_meta<Bool2>::__index_of<bool> == 0);
static_assert(std::__storage_meta<Bool2>::__index_of<Bool2::None> == 1);
static_assert(std::__storage_meta<Amb>::__index_of<int> == 2);            // 匹配多个 → 哨兵

struct RefMix { int& a; int b; const int& c; };                            // 用户例：三种类型精确区分
static_assert(std::__storage_meta<RefMix>::__member_count == 3);
static_assert(std::is_same_v<std::__storage_meta<RefMix>::__member_type<0>, int&>);
static_assert(std::is_same_v<std::__storage_meta<RefMix>::__member_type<1>, int>);
static_assert(std::is_same_v<std::__storage_meta<RefMix>::__member_type<2>, const int&>);
static_assert(std::is_same_v<std::__storage_meta<RefMix>::__member_tuple_t, std::tuple<int&, int, const int&>>);
static_assert(std::__storage_meta<RefMix>::__index_of<int&> == 0);         // a
static_assert(std::__storage_meta<RefMix>::__index_of<int> == 1);          // b
static_assert(std::__storage_meta<RefMix>::__index_of<const int&> == 2);   // c
static_assert(std::__storage_meta<RefMix>::__index_of<const int> == 3);    // 无精确匹配 → 哨兵

// ---- __has_member_type：类型表是否存在精确匹配 ----
static_assert(std::__storage_meta<Ref2>::__has_member_type<int&>);
static_assert(std::__storage_meta<Ref2>::__has_member_type<std::string&>);
static_assert(!std::__storage_meta<Ref2>::__has_member_type<int>);       // 精确匹配：int≠int&
static_assert(!std::__storage_meta<Ref2>::__has_member_type<const int&>);
static_assert(std::__storage_meta<RefMix>::__has_member_type<int&>);
static_assert(std::__storage_meta<RefMix>::__has_member_type<int>);
static_assert(std::__storage_meta<RefMix>::__has_member_type<const int&>);
static_assert(!std::__storage_meta<RefMix>::__has_member_type<const int>);
static_assert(std::__storage_meta<Amb>::__has_member_type<int>);         // 歧义也存在 → true
static_assert(std::__storage_meta<Bool2>::__has_member_type<bool>);
static_assert(std::__storage_meta<Bool2>::__has_member_type<Bool2::None>);

// ---- 辅助using：取T的第I个成员变量类型（索引在前） ----
static_assert(std::is_same_v<std::__member_type_t<0, Ref2>, int&>);
static_assert(std::is_same_v<std::__member_type_t<1, Ref2>, std::string&>);
static_assert(std::is_same_v<std::__member_type_t<0, RefMix>, int&>);
static_assert(std::is_same_v<std::__member_type_t<1, RefMix>, int>);
static_assert(std::is_same_v<std::__member_type_t<2, RefMix>, const int&>);
static_assert(std::is_same_v<std::__member_type_t<0, Bool2>, bool>);
static_assert(std::is_same_v<std::__member_type_t<1, Bool2>, Bool2::None>);

// ---- 返回类型：get_if ----
static_assert(std::is_same_v<decltype(std::declval<std::__storage_base<Ref2>&>().get_if<0>()), int*>);
static_assert(std::is_same_v<decltype(std::declval<std::__storage_base<Ref2>&>().get_if<int&>()), int*>);
static_assert(std::is_same_v<decltype(std::declval<const std::__storage_base<Ref2>&>().get_if<0>()), int*>);      // 引用成员const不穿透
static_assert(std::is_same_v<decltype(std::declval<std::__storage_base<Mixed>&>().get_if<0>()), int*>);
static_assert(std::is_same_v<decltype(std::declval<const std::__storage_base<Mixed>&>().get_if<0>()), const int*>); // 值成员const传播
static_assert(std::is_same_v<decltype(std::declval<std::__storage_base<Bool2>&>().get_if<0>()), bool*>);

// ---- 编译期判定 ----
static_assert(std::__storage_meta<Ref2>::__niche_capable);
static_assert(std::__storage_meta<Ref1>::__niche_capable);
static_assert(std::__storage_meta<Ref3>::__niche_capable);
static_assert(!std::__storage_meta<NoNiche>::__niche_capable);
static_assert(!std::__storage_meta<Mixed>::__niche_capable);
static_assert(!std::__storage_meta<Ref2NoOpt>::__niche_capable);   // 无开关 → 不启用

static_assert(std::__storage_meta<Bool1>::__bool_niche_capable);
static_assert(std::__storage_meta<Bool2>::__bool_niche_capable);
static_assert(std::__storage_meta<Bool3>::__bool_niche_capable);
static_assert(!std::__storage_meta<BoolFail>::__bool_niche_capable);
static_assert(!std::__storage_meta<BoolFail>::__niche_capable);
static_assert(!std::__storage_meta<Bool1NoOpt>::__bool_niche_capable); // 无开关 → 不启用

// ---- 尺寸：niche版恒8字节（无tag字段），tag版按现状 ----
static_assert(sizeof(std::__storage_base<Ref2>) == 8);   // 旧方案16 → 省8
static_assert(sizeof(std::__storage_base<Ref1>) == 8);
static_assert(sizeof(std::__storage_base<Ref3>) == 8);
static_assert(sizeof(std::__storage_base<NoNiche>) == 16);
static_assert(sizeof(std::__storage_base<Mixed>) == 16); // string& 适配为指针8 + tag 1 → 对齐8
static_assert(sizeof(std::__storage_base<Ref2NoOpt>) == 16); // 无开关 → tag版

// ---- 尺寸：bool niche恒1字节（tag版2字节） ----
static_assert(sizeof(std::__storage_base<Bool1>) == 1);
static_assert(sizeof(std::__storage_base<Bool2>) == 1);
static_assert(sizeof(std::__storage_base<Bool3>) == 1);
static_assert(sizeof(std::__storage_base<BoolFail>) == 2); // tag版：byte[1] + tag[1]
static_assert(sizeof(std::__storage_base<Bool1NoOpt>) == 2); // 无开关 → tag版

// ---- 返回类型：引用成员const不穿透 ----
static_assert(std::is_same_v<decltype(std::declval<std::__storage_base<Ref2>&>().get<0>()), int&>);
static_assert(std::is_same_v<decltype(std::declval<std::__storage_base<Ref2>&>().get<1>()), std::string&>);
static_assert(std::is_same_v<decltype(std::declval<const std::__storage_base<Ref2>&>().get<1>()), std::string&>);

// ---- 返回类型：bool成员→真bool&（const对象→const bool&）；空类成员→按值 ----
static_assert(std::is_same_v<decltype(std::declval<std::__storage_base<Bool2>&>().get<0>()), bool&>);
static_assert(std::is_same_v<decltype(std::declval<const std::__storage_base<Bool2>&>().get<0>()), const bool&>);
static_assert(std::is_same_v<decltype(std::declval<std::__storage_base<Bool2>&>().get<1>()), Bool2::None>);

int main() {
  // ============ 通用版（tag）：All 全类型成员 ============
  R<All> rs;
  int iv = 1;
  int iv2 = 2;
  std::string sv = "sv";
  std::string sv2 = "sv2";

  rs.construct<0>(42);            // int
  static_assert(std::is_same_v<decltype(rs.get<0>()), int&>);
  rs.get<0>() = 43;
  rs.destroy<0>();

  rs.construct<1>(7);             // volatile int
  static_assert(std::is_same_v<decltype(rs.get<1>()), volatile int&>);
  rs.destroy<1>();

  rs.construct<2>(8);             // const int
  static_assert(std::is_same_v<decltype(rs.get<2>()), const int&>);
  rs.destroy<2>();

  rs.construct<3>(9);             // volatile const int
  static_assert(std::is_same_v<decltype(rs.get<3>()), const volatile int&>);
  rs.destroy<3>();

  rs.construct<4>(iv);            // int&
  static_assert(std::is_same_v<decltype(rs.get<4>()), int&>);
  rs.get<4>() = 99;
  rs.destroy<4>();
  assert(iv == 99);

  rs.construct<5>(iv2);           // int&& 按左值绑定存指针
  static_assert(std::is_same_v<decltype(rs.get<5>()), int&>);
  rs.destroy<5>();

  rs.construct<6>(iv);            // const int&
  static_assert(std::is_same_v<decltype(rs.get<6>()), const int&>);
  rs.destroy<6>();

  rs.construct<7>(&iv);           // int*
  static_assert(std::is_same_v<decltype(rs.get<7>()), int*&>);
  *rs.get<7>() = 100;
  rs.destroy<7>();
  assert(iv == 100);

  rs.construct<8>(std::string("a"));   // string
  static_assert(std::is_same_v<decltype(rs.get<8>()), std::string&>);
  rs.get<8>() += "b";
  rs.destroy<8>();

  rs.construct<9>(std::string("v"));   // volatile string
  static_assert(std::is_same_v<decltype(rs.get<9>()), volatile std::string&>);
  rs.destroy<9>();

  rs.construct<10>(std::string("c"));  // const string
  static_assert(std::is_same_v<decltype(rs.get<10>()), const std::string&>);
  rs.destroy<10>();

  rs.construct<11>(std::string("vc")); // volatile const string
  static_assert(std::is_same_v<decltype(rs.get<11>()), const volatile std::string&>);
  rs.destroy<11>();

  rs.construct<12>(sv);            // string&
  static_assert(std::is_same_v<decltype(rs.get<12>()), std::string&>);
  rs.get<12>() += "x";
  rs.destroy<12>();
  assert(sv == "svx");

  rs.construct<13>(sv2);           // string&& 按左值绑定
  static_assert(std::is_same_v<decltype(rs.get<13>()), std::string&>);
  rs.destroy<13>();

  rs.construct<14>(sv);            // const string&
  static_assert(std::is_same_v<decltype(rs.get<14>()), const std::string&>);
  rs.destroy<14>();

  rs.construct<15>(&sv);           // string*
  static_assert(std::is_same_v<decltype(rs.get<15>()), std::string*&>);
  rs.get<15>()->append("y");
  rs.destroy<15>();
  assert(sv == "svxy");

  rs.construct<0>(1);              // const 对象访问
  R<All> const& crs = rs;
  static_assert(std::is_same_v<decltype(crs.get<0>()), const int&>);
  static_assert(std::is_same_v<decltype(crs.get<1>()), const volatile int&>);
  static_assert(std::is_same_v<decltype(crs.get<4>()), int&>);        // 引用成员不穿透const
  static_assert(std::is_same_v<decltype(crs.get<5>()), int&>);
  static_assert(std::is_same_v<decltype(crs.get<6>()), const int&>);
  static_assert(std::is_same_v<decltype(crs.get<7>()), int* const&>);
  static_assert(std::is_same_v<decltype(crs.get<8>()), const std::string&>);
  static_assert(std::is_same_v<decltype(crs.get<12>()), std::string&>);
  static_assert(std::is_same_v<decltype(crs.get<14>()), const std::string&>);
  rs.destroy<0>();

  static_assert(std::is_same_v<decltype(std::move(rs).get<0>()), int&&>);
  static_assert(std::is_same_v<decltype(std::move(rs).get<4>()), int&>);  // 引用成员不forward_like
  static_assert(std::is_same_v<decltype(std::move(rs).get<8>()), std::string&&>);

  // 自定义非平凡析构类型
  R<AllW> rw;
  Widget wt;
  wt.tag = 7;
  rw.construct<0>(Widget{});
  rw.destroy<0>();                 // 显式析构被调用
  rw.construct<1>(wt);             // Widget&
  static_assert(std::is_same_v<decltype(rw.get<1>()), Widget&>);
  rw.get<1>().tag = 8;             // 通过引用修改原对象
  rw.destroy<1>();
  assert(wt.tag == 8);
  rw.construct<2>(&wt);            // Widget*
  static_assert(std::is_same_v<decltype(rw.get<2>()), Widget*&>);
  rw.destroy<2>();
  assert(wt.tag == 8);             // 引用成员析构不影响被引用对象

  // ============ niche版：引用niche ============
  int x = 1;
  std::string s = "hello";

  { // 双引用 niche
    std::__storage_base<Ref2> r;
    assert(!r.has_value());
    r.construct<0>(x);
    assert(r.has_value() && r.index() == 0);
    r.get<0>() = 2;
    assert(x == 2);                                // 引用绑定生效
    r.destroy<0>();
    assert(!r.has_value() && r.index() == 2);      // disengaged index == member_count

    r.construct<1>(s);
    assert(r.index() == 1);
    r.get<1>() += "!";
    assert(s == "hello!");                         // 修改透过引用
    const auto& cr = r;
    assert(cr.get<1>() == "hello!");               // const对象get：const不穿透
    r.destroy<1>();
    assert(!r.has_value());
  }

  { // 单引用 niche（mask=0 退化路径）
    std::__storage_base<Ref1> r;
    r.construct<0>(x);
    assert(r.index() == 0 && r.get<0>() == 2);
    r.destroy<0>();
    assert(r.index() == 1);
  }

  { // 三引用 niche
    double d = 3.14;
    std::__storage_base<Ref3> r;
    r.construct<2>(d);
    assert(r.index() == 2 && r.get<2>() == 3.14);
    r.destroy<2>();
    assert(r.index() == 3);
  }

  { // 对齐不足 → tag版回退（引用分支仍正常）
    char c = 'z';
    std::__storage_base<NoNiche> r;
    r.construct<1>(c);
    assert(r.index() == 1 && r.get<1>() == 'z');
    r.destroy<1>();
    assert(r.index() == 2);
  }

  { // 含值成员 → tag版回退
    std::__storage_base<Mixed> r;
    r.construct<0>(42);
    assert(r.index() == 0 && r.get<0>() == 42);
    r.destroy<0>();
    std::string s2 = "world";
    r.construct<1>(s2);
    assert(r.index() == 1 && r.get<1>() == "world");
    r.destroy<1>();
    assert(r.index() == 2);
  }

  { // bool niche：bool + 空类
    std::__storage_base<Bool2> r;
    assert(!r.has_value() && r.index() == 2);
    r.construct<0>(true);
    assert(r.has_value() && r.index() == 0);
    assert(r.get<0>() == true);
    r.get<0>() = false;                        // bool& 可写，编码不破坏
    assert(r.get<0>() == false);
    const auto& cr = r;
    assert(cr.get<0>() == false);              // const对象 → const bool&
    r.destroy<0>();
    assert(!r.has_value() && r.index() == 2);

    r.construct<1>();                          // 空变体：无参（unit语义）
    assert(r.has_value() && r.index() == 1);
    [[maybe_unused]] Bool2::None n = r.get<1>(); // 按值返回
    r.destroy<1>();
    assert(!r.has_value() && r.index() == 2);
  }

  { // bool + 2空类：空变体编码 2/3、index()反查
    std::__storage_base<Bool3> r;
    r.construct<2>();
    assert(r.index() == 2);
    r.destroy<2>();
    r.construct<1>();
    assert(r.index() == 1);
    r.destroy<1>();
    r.construct<0>(true);
    assert(r.index() == 0 && r.get<0>());
    r.destroy<0>();
    assert(r.index() == 3);
  }

  { // 单bool：optional<bool> 语义，1字节
    std::__storage_base<Bool1> r;
    assert(!r.has_value() && r.index() == 1);
    r.construct<0>(false);
    assert(r.index() == 0 && r.get<0>() == false);
    r.destroy<0>();
    assert(!r.has_value() && r.index() == 1);
  }

  { // 双bool：值域重叠 → tag版回退
    std::__storage_base<BoolFail> r;
    r.construct<1>(true);
    assert(r.index() == 1 && r.get<1>() == true);
    r.destroy<1>();
    assert(r.index() == 2);
  }

  { // 按类型get：引用niche版（转发按索引get，精确匹配）
    std::__storage_base<Ref2> r;
    int x = 5;
    std::string s = "abc";
    r.construct<0>(x);
    assert(r.get<int&>() == 5);
    r.get<int&>() = 6;                             // 精确匹配引用成员，可写
    assert(x == 6);
    const auto& cr = r;
    assert(cr.get<int&>() == 6);                   // const不穿透引用成员，仍返回int&
    r.destroy<0>();
    r.construct<1>(s);
    assert(r.get<std::string&>() == "abc");
    r.destroy<1>();
  }

  { // 按类型get：tag版（值成员）
    std::__storage_base<Mixed> m;
    m.construct<0>(42);
    assert(m.get<int>() == 42);
    m.destroy<0>();

    std::__storage_base<Bool2> b;
    b.construct<0>(true);
    assert(b.get<bool>() == true);
    b.destroy<0>();
    b.construct<1>();
    [[maybe_unused]] Bool2::None n2 = b.get<Bool2::None>(); // 空变体按值返回
    b.destroy<1>();
  }

  { // get_if：安全查询路径（不匹配返回nullptr，不触发断言）
    std::__storage_base<Mixed> m;
    m.construct<0>(42);
    assert(m.get_if<0>() != nullptr && *m.get_if<0>() == 42);
    assert(m.get_if<1>() == nullptr);                // 非活动变体 → nullptr
    m.destroy<0>();
    assert(m.get_if<0>() == nullptr);                // disengaged → nullptr
    m.construct<0>(7);
    assert(*m.get_if<int>() == 7);                   // 按类型get_if
    m.destroy<0>();

    std::__storage_base<Ref2> r;
    int x = 9;
    r.construct<0>(x);
    assert(r.get_if<int&>() == &x);                  // 引用成员 → 指向被引用对象
    r.destroy<0>();
    assert(r.get_if<0>() == nullptr);

    std::__storage_base<Bool2> b;
    b.construct<0>(true);
    assert(b.get_if<bool>() != nullptr && *b.get_if<bool>());
    b.destroy<0>();
    assert(b.get_if<0>() == nullptr);
  }

  { // 精确匹配区分三种类型（用户例：int&/int/const int&）
    std::__storage_base<RefMix> r;
    int x = 1;
    const int z = 3;
    r.construct<0>(x);
    assert(r.get<int&>() == 1);
    r.destroy<0>();
    r.construct<1>(42);
    assert(r.get<int>() == 42);
    r.destroy<1>();
    r.construct<2>(z);
    assert(r.get<const int&>() == 3);
    r.destroy<2>();
  }

  { // 无开关 → tag版回退（默认关闭）
    std::__storage_base<Ref2NoOpt> r;
    int x = 7;
    r.construct<0>(x);
    assert(r.index() == 0 && r.get<0>() == 7);
    r.destroy<0>();
    assert(r.index() == 2);
  }

  std::puts("ALL PASS");
}