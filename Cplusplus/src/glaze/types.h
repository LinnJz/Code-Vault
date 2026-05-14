inline void
cpp_types()
{
  bool                        bool_type                 = true;
  char                        char_type                 = 'a';
  signed char                 char_signed_type          = -128;
  unsigned char               char_unsigned_type        = 255;
  wchar_t                     wchar_type                = L'a';
  char8_t                     char8_type                = u8'a';
  char16_t                    char16_type               = u'a';
  char32_t                    char32_type               = U'a';
  short                       short_type                = -32768;
  unsigned short              ushort_type               = 65535;
  int                         int_type                  = -2147483648;
  unsigned int                uint_type                 = 4294967295;
  long                        long_type                 = -9223372036854775807L;
  unsigned long               ulong_type                = 18446744073709551615UL;
  long long                   long_long_type            = -9223372036854775807LL;
  unsigned long long          ulong_long_type           = 18446744073709551615ULL;
  float                       float_type                = 3.14f;
  double                      double_type               = 3.14;
  long double                 long_double_type          = 3.14l;
  std::complex<double>        complex_double_type       = { 1.0, 2.0 };
  void                       *void_type_ptr             = nullptr;
  const void                 *const_void_type_ptr       = nullptr;
  const void *const           const_void_type_ptr_const = nullptr;
  std::nullptr_t              nullptr_type              = nullptr;
  std::nullopt_t              nullopt_type              = std::nullopt;
  std::nothrow_t              nothrow_type              = std::nothrow;
  std::monostate              monostate_type            = std::monostate();
  volatile std::size_t        volatile_size_t_type      = std::size_t();
  std::size_t                 size_t_type               = std::size_t();
  std::size_t                &size_t_ref_type           = size_t_type;
  const std::size_t           const_size_t_type         = std::size_t();
  const std::size_t          &const_size_t_ref_type     = const_size_t_type;
  std::size_t               &&size_t_rvalue_ref_type    = std::size_t();
  std::reference_wrapper<int> ref_wrapper_type          = std::ref(int_type);

  char char_array_type[3]     = { 'a', 'b', 'c' };
  int  multi_array_type[2][3] = {
    { 1, 2, 3 },
    { 4, 5, 6 }
  };

  enum class Color
  {
    RED,
    GREEN,
    BLUE
  };
  Color color_type = Color::RED;

  enum Direction
  {
    UP,
    DOWN,
    LEFT,
    RIGHT
  };

  Direction direction_type = UP;

  union UnionExample
  {
    int   i;
    float f;
    char  c;
  };

  UnionExample union_type = { .i = 10 };

  struct Point
  {
    double x, y;
  };

  Point point_type = { 1.0, 2.0 };

  class Base
  {
  public:
    Base(const std::string &str)
        : str(str)
    {
    }

    virtual ~Base() = default;

    static std::string_view get_type_name() noexcept { return "MyClass"; }

  protected:
    std::string str;
  };

  class Derived : public Base
  {
  public:
    Derived(int num, const std::string &str)
        : Base(str)
        , num(num)
    {
    }

    ~Derived() override = default;

  private:
    int num;
  };

  Base    base_type("hello");
  Derived derived_type(10, "world");
  Base   *derived_ptr_type = &derived_type;

  struct Red
  {
    uint8_t r;
  };

  struct Green
  {
    uint8_t g;
  };

  struct Blue
  {
    uint8_t b;
  };

  struct RGB
      : public Red
      , Green
      , Blue
  {
  };

  RGB    rgb_type       = { 255, 0, 0 };
  RGB   *rgb_type_ptr   = &rgb_type;
  Red   *red_type_ptr   = static_cast<Red *>(rgb_type_ptr);
  Green *green_type_ptr = static_cast<Green *>(rgb_type_ptr);
  Blue  *blue_type_ptr  = static_cast<Blue *>(rgb_type_ptr);

  std::unique_ptr<RGB>       rgb_unique_ptr_type = std::make_unique<RGB>();
  std::shared_ptr<RGB>       rgb_shared_ptr_type = std::make_shared<RGB>();
  std::weak_ptr<RGB>         rgb_weak_ptr_type   = rgb_shared_ptr_type;
  std::mutex                 mutex_type;
  std::timed_mutex           timed_mutex_type;
  std::recursive_mutex       recursive_mutex_type;
  std::recursive_timed_mutex recursive_timed_mutex_type;
  std::shared_mutex          shared_mutex_type;
  std::shared_timed_mutex    shared_timed_mutex_type;

  std::lock_guard<std::mutex>         lock_guard_type(mutex_type);
  std::unique_lock<std::timed_mutex>  unique_lock_type(timed_mutex_type);
  std::shared_lock<std::shared_mutex> shared_lock_type(shared_mutex_type);

  std::condition_variable     condition_variable_type;
  std::condition_variable_any condition_variable_any_type;

  std::atomic_bool atomic_bool_type = true;
  std::atomic_char atomic_char_type = 'a';
  std::atomic_flag atomic_flag_type = ATOMIC_FLAG_INIT;

  std::byte                byte_type             = std::byte(0);
  std::byte                byte_array_type[3]    = { std::byte(0), std::byte(1), std::byte(2) };
  std::initializer_list    init_list_int_type    = { 1, 2, 3 };
  std::initializer_list    init_list_string_type = { "hello", "world" };
  std::array<int, 3>       array_int_type        = { 1, 2, 3 };
  std::vector<std::string> vector_string_type    = { "hello", "world" };
  std::deque<int>          deque_int_type        = { 1, 2, 3 };
  std::list<int>           list_int_type         = { 1, 2, 3 };
  std::forward_list<int>   forward_list_int_type = { 1, 2, 3 };
  std::stack<int>          stack_int_type(array_int_type.begin(), array_int_type.end());
  std::queue<int>          queue_int_type(array_int_type.begin(), array_int_type.end());
  std::priority_queue<int> priority_queue_int_type(array_int_type.begin(), array_int_type.end());
  std::span<int>           span_int_type     = array_int_type;
  std::bitset<8>           bitset_type       = 0b10'101'010;
  std::valarray<int>       valarray_int_type = { 1, 2, 3 };

  std::string      string_type      = "hello world";
  std::string_view string_view_type = "hello world";
  std::wstring     wstring_type     = L"hello world";
  std::u8string    u8string_type    = u8"hello world";
  std::u16string   u16string_type   = u"hello world";
  std::u32string   u32string_type   = U"hello world";

  std::map<std::string, int> map_string_int_type = {
    { "hello", 1 },
    { "world", 2 }
  };
  std::unordered_map<std::string, int> unordered_map_string_int_type = {
    { "hello", 1 },
    { "world", 2 }
  };
  std::set<int>                   set_int_type             = { 1, 2, 3 };
  std::unordered_set<int>         unordered_set_int_type   = { 1, 2, 3 };
  std::multimap<std::string, int> multimap_string_int_type = {
    { "hello", 1 },
    { "world", 2 }
  };
  std::unordered_multimap<std::string, int> unordered_multimap_string_int_type = {
    { "hello", 1 },
    { "world", 2 }
  };
  std::multiset<int>           multiset_int_type           = { 1, 2, 3 };
  std::unordered_multiset<int> unordered_multiset_int_type = { 1, 2, 3 };

  std::istream_iterator<int>                          istream_iterator_type(std::cin);
  std::ostream_iterator<int>                          ostream_iterator_type(std::cout);
  std::reverse_iterator<std::vector<int>::iterator>   reverse_iterator_type;
  std::back_insert_iterator<std::vector<std::string>> back_insert_iterator_type(vector_string_type);
  std::move_iterator<std::vector<int>::iterator>      move_iterator_type;

  std::ranges::subrange<std::vector<int>::iterator> subrange_type;
  std::ranges::ref_view<std::vector<std::string>>   ref_view_type(vector_string_type);
  std::ranges::owning_view<std::vector<int>>        owning_view_type;
  std::ranges::empty_view<int>                      empty_view_type;

  struct Person
  {
    int         age;
    double      weight;
    std::string name;
  };

  std::tuple<int, double, std::string, char, bool, long, Person, Base *, std::unique_ptr<char[]>, std::shared_ptr<RGB>>
      tuple_type = {
        10,
        3.14,
        "hello",
        'a',
        true,
        1000L,
        Person { 25, 60.5, "John" },
        &base_type,
        std::make_unique<char[]>(10),
        std::make_shared<RGB>()
  };
  std::pair<std::string, RGB *> pair_type = { "hello", &rgb_type };
  std::variant<int, double, std::string, char, bool, long, Person, Base *, std::unique_ptr<char[]>,
               std::shared_ptr<RGB>>
                       variant_type         = std::make_shared<RGB>();
  std::any             any_type             = std::make_shared<RGB>();
  std::optional<int>   optional_int_type    = 10;
  std::strong_ordering strong_ordering_type = std::strong_ordering::equal;

  std::stringstream          stringstream_type;
  std::ifstream              input_file_stream_type("input.txt");
  std::ofstream              output_file_stream_type("output.txt");
  std::fstream               file_stream_type;
  std::iostream             *iostream_type        = &stringstream_type;
  std::pmr::memory_resource *memory_resource_type = std::pmr::get_default_resource();
  std::allocator<int>        allocator_type;

  std::chrono::system_clock::time_point time_point_type = std::chrono::system_clock::now();
  std::chrono::steady_clock::time_point steady_clock_time_point_type;
  std::chrono::duration<double>         duration_type = std::chrono::seconds(1);

  std::regex                                       regex_type = std::regex("hello.*world");
  std::smatch                                      smatch_type;
  std::regex_iterator<std::string::iterator>       regex_iterator_type;
  std::regex_token_iterator<std::string::iterator> regex_token_iterator_type;

  std::filesystem::path            path_type = "C:/Users/John/Documents";
  std::filesystem::directory_entry directory_entry_type;
  std::filesystem::file_status     file_status_type;

  std::formatter<std::string> formatter_type;

  std::error_code      error_code_type;
  std::error_condition error_condition_type;
  std::exception_ptr   exception_ptr_type;
  std::runtime_error   runtime_error_type("error");

  std::integral_constant<int, 5>     integral_constant_type;
  std::mt19937                       mt19937_type;
  std::random_device                 random_device_type;
  std::uniform_int_distribution<int> uniform_int_dist_type(0, 100);
  std::normal_distribution<double>   normal_dist_type;
  std::seed_seq                      seed_seq_type;

  std::thread                  thread_type([](int a)
  {
    std::cout << "Thread: " << a << std::endl;
  }, 10);
  std::jthread                 jthread_type;
  std::future<int>             future_int_type    = std::async(std::launch::async, []()
                 {
    return 10;
  });
  std::promise<int>            promise_int_type   = std::promise<int>();
  std::shared_future<int>      shared_future_type = future_int_type.share();
  std::packaged_task<int(int)> packaged_task_int_type([](int a)
  {
    return a * 2;
  });

  std::function<std::string(int, double, std::string)> func_type = [](int a, double b, std::string c)
  {
    return std::to_string(a) + " " + std::to_string(b) + " " + c;
  };
  int (*func_ptr_type)(int, double)                       = nullptr;
  double Point::*member_ptr_type                          = &Point::x;
  void (Base::*member_func_ptr_type)(const std::string &) = nullptr;
}
