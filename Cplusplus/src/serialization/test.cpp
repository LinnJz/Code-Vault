/*
Run on (8 X 2400 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)
-----------------------------------------------------------------
Benchmark                       Time             CPU   Iterations
-----------------------------------------------------------------
BM_ProtobufSerialize        41664 ns        39307 ns        19478
BM_ProtobufDeserialize     220346 ns       219717 ns         2489
BM_YYJSONSerialize          36988 ns        36901 ns        19478
BM_YYJSONDeserialize        26467 ns        26681 ns        26353
*/

#include <benchmark/benchmark.h>
#include <google/protobuf/util/json_util.h>
#include <yyjson.h>

#include <iostream>
#include <map>
#include <random>
#include <string>
#include <vector>

#include "organization.pb.h"

// 确保 Protobuf 库初始化
static bool pb_initialized = []() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  return true;
}();

// 随机数生成器
static std::mt19937                            rng(std::random_device {}());
static std::uniform_int_distribution<uint32_t> dist_id(1, 10000);
static std::uniform_real_distribution<float>   dist_salary(30000.0f, 150000.0f);
static std::uniform_int_distribution<int>      dist_year(2000, 2025);
static std::uniform_int_distribution<int>      dist_month(1, 12);
static std::uniform_int_distribution<int>      dist_day(1, 28);
static std::uniform_int_distribution<int>      dist_performance(1, 100);
static std::uniform_int_distribution<int>      dist_choice(0, 2);
static std::uniform_int_distribution<int>      dist_byte(0, 255);

// 辅助函数：生成随机日期字符串
std::string random_date()
{
  int  y = dist_year(rng);
  int  m = dist_month(rng);
  int  d = dist_day(rng);
  char buf [11];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d", y, m, d);
  return buf;
}

// 辅助函数：生成随机 bytes
std::string random_bytes(size_t len)
{
  std::string data(len, '\0');
  for (size_t i = 0; i < len; ++i) {
    data [i] = static_cast<char>(dist_byte(rng));
  }
  return data;
}

// ------------------------------------------------------------
// 构建一个巨大的 Organization 对象
// ------------------------------------------------------------
benchmark::Organization CreateComplexOrganization()
{
  benchmark::Organization org;
  org.set_name("MegaCorp International");
  org.set_id(123456789);
  org.set_founded_date("1985-05-15");

  // 总部地址
  auto* hq = org.mutable_headquarters();
  hq->set_street("1 Infinite Loop");
  hq->set_city("Cupertino");
  hq->set_state("CA");
  hq->set_zip("95014");
  hq->set_country("USA");

  // 公司标签
  (*org.mutable_tags()) ["industry"] = "technology";
  (*org.mutable_tags()) ["stock"]    = "MEGA";
  (*org.mutable_tags()) ["ceo"]      = "John Doe";

  // 创建 3 个部门
  for (int d = 1; d <= 3; ++d) {
    auto* dept = org.add_departments();
    dept->set_name("Department " + std::to_string(d));
    dept->set_id(d * 100);
    dept->set_floor(d + 1);
    (*dept->mutable_metadata()) ["budget"] =
        std::to_string(5'000'000 + d * 1'000'000);
    (*dept->mutable_metadata()) ["headcount_goal"] = std::to_string(50 + d * 10);

    // 每个部门创建 2 个团队
    for (int t = 1; t <= 2; ++t) {
      auto* team = dept->add_teams();
      team->set_name("Team " + std::to_string(t) + " (Dept " + std::to_string(d) +
                     ")");
      team->set_id(d * 1000 + t);

      // 团队 leader
      auto* leader = team->mutable_leader();
      leader->set_id(dist_id(rng));
      leader->set_name("Leader " + std::to_string(leader->id()));
      leader->set_email("leader" + std::to_string(leader->id()) +
                        "@megacorp.com");
      leader->set_salary(dist_salary(rng));
      leader->set_hire_date(random_date());
      leader->add_addresses()->set_street("123 Manager Lane");
      leader->add_phone_numbers("555-0001");
      (*leader->mutable_attributes()) ["level"] = "senior";
      leader->mutable_emergency_contact()->set_name("Spouse");
      leader->add_roles("manager");
      leader->add_performance_scores(dist_performance(rng) / 100.0f);
      leader->set_metadata(random_bytes(64));  // 64 字节二进制数据

      // 团队成员：每个团队 5 名普通员工
      for (int e = 1; e <= 5; ++e) {
        auto* emp = team->add_members();
        emp->set_id(dist_id(rng));
        emp->set_name("Employee " + std::to_string(emp->id()));
        emp->set_email("emp" + std::to_string(emp->id()) + "@megacorp.com");
        emp->set_salary(dist_salary(rng));
        emp->set_hire_date(random_date());

        // 每个员工 2 个地址
        for (int a = 0; a < 2; ++a) {
          auto* addr = emp->add_addresses();
          addr->set_street(std::to_string(100 + a) + " Main St");
          addr->set_city("City" + std::to_string(emp->id()));
          addr->set_state("CA");
          addr->set_zip("9000" + std::to_string(a));
          addr->set_country("USA");
        }

        // 3 个电话号码
        emp->add_phone_numbers("555-1234");
        emp->add_phone_numbers("555-5678");
        emp->add_phone_numbers("555-9012");

        // 5 个属性
        (*emp->mutable_attributes()) ["skill"] = (e % 2 == 0) ? "C++" : "Python";
        (*emp->mutable_attributes()) ["level"] = std::to_string(e % 3 + 1);
        (*emp->mutable_attributes()) ["team"]  = team->name();
        (*emp->mutable_attributes()) ["department"] = dept->name();
        (*emp->mutable_attributes()) ["city"]       = "Anytown";

        // 紧急联系人
        auto* contact = emp->mutable_emergency_contact();
        contact->set_name("Emergency Contact " + std::to_string(emp->id()));
        contact->set_relationship("Family");
        contact->set_phone("555-9999");

        // 角色
        emp->add_roles("developer");
        if (e == 0) emp->add_roles("lead");

        // 绩效分数
        for (int p = 0; p < 4; ++p) {
          emp->add_performance_scores(dist_performance(rng) / 100.0f);
        }

        // 二进制元数据
        emp->set_metadata(random_bytes(128));  // 128 字节
      }

      // 每个团队创建 2 个项目
      for (int p = 1; p <= 2; ++p) {
        auto* proj = team->add_projects();
        proj->set_name("Project " + std::to_string(p) + " (Team " +
                       std::to_string(t) + ")");
        proj->set_id(p * 10000 + t);
        proj->set_description(
            "This is a very long description that simulates real-world project documentation. It contains multiple sentences and is intended to increase the size of the message.");
        proj->set_start_date(random_date());
        if (p % 2 == 0) {
          proj->set_end_date(random_date());  // 部分项目有结束日期
        }

        // 每个项目创建 3 个任务
        for (int task_id = 1; task_id <= 3; ++task_id) {
          auto* task = proj->add_tasks();
          task->set_title("Task " + std::to_string(task_id));
          task->set_description(
              "Detailed task description with specific instructions and acceptance criteria.");
          task->set_status(
              static_cast<benchmark::TaskStatus>(dist_choice(rng)));  // 随机状态
          task->set_assigned_employee_id(
              team->members(task_id % team->members_size())
                  .id());  // 随机分配一个员工

          // 每个任务添加 2 条评论
          for (int c = 0; c < 2; ++c) {
            auto* comment = task->add_comments();
            comment->set_author("user" + std::to_string(dist_id(rng)));
            comment->set_text(
                "This is a sample comment that could be left by a team member during code review or daily standup.");
            comment->set_timestamp(static_cast<uint64_t>(time(nullptr)) -
                                   86400 * c);
          }
        }
      }
    }
  }

  return org;
}

// ------------------------------------------------------------
// 根据 Protobuf Organization 构建 yyjson 文档
// ------------------------------------------------------------
yyjson_mut_doc* BuildYYDocFromOrganization(const benchmark::Organization& org)
{
  yyjson_mut_doc* doc  = yyjson_mut_doc_new(nullptr);
  yyjson_mut_val* root = yyjson_mut_obj(doc);
  yyjson_mut_doc_set_root(doc, root);

  // 标量字段
  yyjson_mut_obj_add_str(doc, root, "name", org.name().c_str());
  yyjson_mut_obj_add_uint(doc, root, "id", org.id());
  yyjson_mut_obj_add_str(doc, root, "founded_date", org.founded_date().c_str());

  // 总部地址
  const auto&     hq     = org.headquarters();
  yyjson_mut_val* hq_obj = yyjson_mut_obj(doc);
  yyjson_mut_obj_add_str(doc, hq_obj, "street", hq.street().c_str());
  yyjson_mut_obj_add_str(doc, hq_obj, "city", hq.city().c_str());
  yyjson_mut_obj_add_str(doc, hq_obj, "state", hq.state().c_str());
  yyjson_mut_obj_add_str(doc, hq_obj, "zip", hq.zip().c_str());
  yyjson_mut_obj_add_str(doc, hq_obj, "country", hq.country().c_str());
  yyjson_mut_obj_add_val(doc, root, "headquarters", hq_obj);

  // 标签
  yyjson_mut_val* tags = yyjson_mut_obj(doc);
  for (const auto& [key, val] : org.tags()) {
    yyjson_mut_obj_add_str(doc, tags, key.c_str(), val.c_str());
  }
  yyjson_mut_obj_add_val(doc, root, "tags", tags);

  // 部门数组
  yyjson_mut_val* depts = yyjson_mut_arr(doc);
  for (const auto& dept : org.departments()) {
    yyjson_mut_val* dept_obj = yyjson_mut_obj(doc);
    yyjson_mut_obj_add_str(doc, dept_obj, "name", dept.name().c_str());
    yyjson_mut_obj_add_uint(doc, dept_obj, "id", dept.id());
    yyjson_mut_obj_add_uint(doc, dept_obj, "floor", dept.floor());

    // 部门元数据
    yyjson_mut_val* metadata = yyjson_mut_obj(doc);
    for (const auto& [key, val] : dept.metadata()) {
      yyjson_mut_obj_add_str(doc, metadata, key.c_str(), val.c_str());
    }
    yyjson_mut_obj_add_val(doc, dept_obj, "metadata", metadata);

    // 团队数组
    yyjson_mut_val* teams = yyjson_mut_arr(doc);
    for (const auto& team : dept.teams()) {
      yyjson_mut_val* team_obj = yyjson_mut_obj(doc);
      yyjson_mut_obj_add_str(doc, team_obj, "name", team.name().c_str());
      yyjson_mut_obj_add_uint(doc, team_obj, "id", team.id());

      // 递归函数：处理员工对象（leader 和 members 共用）
      auto add_employee = [&](yyjson_mut_doc* doc, yyjson_mut_val* parent,
                              const benchmark::Employee& emp) {
        yyjson_mut_obj_add_uint(doc, parent, "id", emp.id());
        yyjson_mut_obj_add_str(doc, parent, "name", emp.name().c_str());
        yyjson_mut_obj_add_str(doc, parent, "email", emp.email().c_str());
        yyjson_mut_obj_add_real(doc, parent, "salary", emp.salary());
        yyjson_mut_obj_add_str(doc, parent, "hire_date", emp.hire_date().c_str());

        // 地址数组
        yyjson_mut_val* addrs = yyjson_mut_arr(doc);
        for (const auto& addr : emp.addresses()) {
          yyjson_mut_val* addr_obj = yyjson_mut_obj(doc);
          yyjson_mut_obj_add_str(doc, addr_obj, "street", addr.street().c_str());
          yyjson_mut_obj_add_str(doc, addr_obj, "city", addr.city().c_str());
          yyjson_mut_obj_add_str(doc, addr_obj, "state", addr.state().c_str());
          yyjson_mut_obj_add_str(doc, addr_obj, "zip", addr.zip().c_str());
          yyjson_mut_obj_add_str(doc, addr_obj, "country",
                                 addr.country().c_str());
          yyjson_mut_arr_add_val(addrs, addr_obj);
        }
        yyjson_mut_obj_add_val(doc, parent, "addresses", addrs);

        // 电话号码数组
        yyjson_mut_val* phones = yyjson_mut_arr(doc);
        for (const auto& phone : emp.phone_numbers()) {
          yyjson_mut_arr_add_str(doc, phones, phone.c_str());
        }
        yyjson_mut_obj_add_val(doc, parent, "phone_numbers", phones);

        // 属性
        yyjson_mut_val* attrs = yyjson_mut_obj(doc);
        for (const auto& [key, val] : emp.attributes()) {
          yyjson_mut_obj_add_str(doc, attrs, key.c_str(), val.c_str());
        }
        yyjson_mut_obj_add_val(doc, parent, "attributes", attrs);

        // 紧急联系人
        const auto&     contact     = emp.emergency_contact();
        yyjson_mut_val* contact_obj = yyjson_mut_obj(doc);
        yyjson_mut_obj_add_str(doc, contact_obj, "name", contact.name().c_str());
        yyjson_mut_obj_add_str(doc, contact_obj, "relationship",
                               contact.relationship().c_str());
        yyjson_mut_obj_add_str(doc, contact_obj, "phone",
                               contact.phone().c_str());
        yyjson_mut_obj_add_val(doc, parent, "emergency_contact", contact_obj);

        // 角色数组
        yyjson_mut_val* roles = yyjson_mut_arr(doc);
        for (const auto& role : emp.roles()) {
          yyjson_mut_arr_add_str(doc, roles, role.c_str());
        }
        yyjson_mut_obj_add_val(doc, parent, "roles", roles);

        // 绩效分数数组
        yyjson_mut_val* scores = yyjson_mut_arr(doc);
        for (float score : emp.performance_scores()) {
          yyjson_mut_arr_add_real(doc, scores, score);
        }
        yyjson_mut_obj_add_val(doc, parent, "performance_scores", scores);

        // bytes 字段：需要 base64 或直接存储为字符串（但 bytes 可能包含非 UTF8
        // 数据，此处简化：用 hex 或 base64 会增大开销，这里仅存原始字符串，yyjson
        // 可能不支持直接 bytes，所以转成 base64 字符串） 为了公平比较，我们将
        // bytes 转成 base64
        // 编码的字符串，反序列化时也需要解码。但为了简化，我们使用原始字符串（假设数据是
        // ASCII），实际 bytes
        // 可能是随机的，但为了测试序列化/反序列化开销，我们可以将其视为字符串。
        // 这里我们用 hex 编码，简单实现。
        auto bytes_to_hex = [](const std::string& bytes) -> std::string {
          static const char* hex = "0123456789ABCDEF";
          std::string        result;
          result.reserve(bytes.size() * 2);
          for (unsigned char c : bytes) {
            result.push_back(hex [c >> 4]);
            result.push_back(hex [c & 0xF]);
          }
          return result;
        };
        yyjson_mut_obj_add_str(doc, parent, "metadata",
                               bytes_to_hex(emp.metadata()).c_str());
      };

      // Leader
      yyjson_mut_val* leader_obj = yyjson_mut_obj(doc);
      add_employee(doc, leader_obj, team.leader());
      yyjson_mut_obj_add_val(doc, team_obj, "leader", leader_obj);

      // 成员数组
      yyjson_mut_val* members = yyjson_mut_arr(doc);
      for (const auto& emp : team.members()) {
        yyjson_mut_val* emp_obj = yyjson_mut_obj(doc);
        add_employee(doc, emp_obj, emp);
        yyjson_mut_arr_add_val(members, emp_obj);
      }
      yyjson_mut_obj_add_val(doc, team_obj, "members", members);

      // 项目数组
      yyjson_mut_val* projects = yyjson_mut_arr(doc);
      for (const auto& proj : team.projects()) {
        yyjson_mut_val* proj_obj = yyjson_mut_obj(doc);
        yyjson_mut_obj_add_str(doc, proj_obj, "name", proj.name().c_str());
        yyjson_mut_obj_add_uint(doc, proj_obj, "id", proj.id());
        yyjson_mut_obj_add_str(doc, proj_obj, "description",
                               proj.description().c_str());
        yyjson_mut_obj_add_str(doc, proj_obj, "start_date",
                               proj.start_date().c_str());
        if (proj.has_end_date()) {
          yyjson_mut_obj_add_str(doc, proj_obj, "end_date",
                                 proj.end_date().c_str());
        }

        // 任务数组
        yyjson_mut_val* tasks = yyjson_mut_arr(doc);
        for (const auto& task : proj.tasks()) {
          yyjson_mut_val* task_obj = yyjson_mut_obj(doc);
          yyjson_mut_obj_add_str(doc, task_obj, "title", task.title().c_str());
          yyjson_mut_obj_add_str(doc, task_obj, "description",
                                 task.description().c_str());
          yyjson_mut_obj_add_uint(doc, task_obj, "status",
                                  static_cast<uint32_t>(task.status()));
          yyjson_mut_obj_add_uint(doc, task_obj, "assigned_employee_id",
                                  task.assigned_employee_id());

          // 评论数组
          yyjson_mut_val* comments = yyjson_mut_arr(doc);
          for (const auto& comment : task.comments()) {
            yyjson_mut_val* comment_obj = yyjson_mut_obj(doc);
            yyjson_mut_obj_add_str(doc, comment_obj, "author",
                                   comment.author().c_str());
            yyjson_mut_obj_add_str(doc, comment_obj, "text",
                                   comment.text().c_str());
            yyjson_mut_obj_add_uint(doc, comment_obj, "timestamp",
                                    comment.timestamp());
            yyjson_mut_arr_add_val(comments, comment_obj);
          }
          yyjson_mut_obj_add_val(doc, task_obj, "comments", comments);

          yyjson_mut_arr_add_val(tasks, task_obj);
        }
        yyjson_mut_obj_add_val(doc, proj_obj, "tasks", tasks);

        yyjson_mut_arr_add_val(projects, proj_obj);
      }
      yyjson_mut_obj_add_val(doc, team_obj, "projects", projects);

      yyjson_mut_arr_add_val(teams, team_obj);
    }
    yyjson_mut_obj_add_val(doc, dept_obj, "teams", teams);

    yyjson_mut_arr_add_val(depts, dept_obj);
  }
  yyjson_mut_obj_add_val(doc, root, "departments", depts);

  return doc;
}

// ------------------------------------------------------------
// 基准测试：Protobuf 序列化
// ------------------------------------------------------------
static void BM_ProtobufSerialize(benchmark::State& state)
{
  auto        org = CreateComplexOrganization();
  std::string serialized;
  for (auto _ : state) {
    org.SerializeToString(&serialized);
    benchmark::DoNotOptimize(serialized.data());
    benchmark::ClobberMemory();
  }
}

BENCHMARK(BM_ProtobufSerialize);

// ------------------------------------------------------------
// 基准测试：Protobuf 反序列化
// ------------------------------------------------------------
static void BM_ProtobufDeserialize(benchmark::State& state)
{
  auto        org = CreateComplexOrganization();
  std::string serialized;
  org.SerializeToString(&serialized);

  for (auto _ : state) {
    benchmark::Organization parsed;
    parsed.ParseFromString(serialized);
    benchmark::DoNotOptimize(parsed);
  }
}

BENCHMARK(BM_ProtobufDeserialize);

// ------------------------------------------------------------
// 基准测试：yyjson 序列化
// ------------------------------------------------------------
static void BM_YYJSONSerialize(benchmark::State& state)
{
  auto            org = CreateComplexOrganization();
  yyjson_mut_doc* doc = BuildYYDocFromOrganization(org);

  for (auto _ : state) {
    size_t len;
    char*  json = yyjson_mut_write(doc, 0, &len);
    benchmark::DoNotOptimize(json);
    benchmark::ClobberMemory();
    free(json);
  }

  yyjson_mut_doc_free(doc);
}

BENCHMARK(BM_YYJSONSerialize);

// ------------------------------------------------------------
// 基准测试：yyjson 反序列化
// ------------------------------------------------------------
static void BM_YYJSONDeserialize(benchmark::State& state)
{
  auto            org = CreateComplexOrganization();
  yyjson_mut_doc* doc = BuildYYDocFromOrganization(org);
  size_t          json_len;
  char*           json = yyjson_mut_write(doc, 0, &json_len);
  yyjson_mut_doc_free(doc);

  for (auto _ : state) {
    yyjson_doc* parsed = yyjson_read(json, json_len, 0);
    benchmark::DoNotOptimize(parsed);
    yyjson_doc_free(parsed);
  }

  free(json);
}

BENCHMARK(BM_YYJSONDeserialize);

// ------------------------------------------------------------
// 主函数
// ------------------------------------------------------------
int main(int argc, char** argv)
{
  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
  ::benchmark::RunSpecifiedBenchmarks();
  ::benchmark::Shutdown();
  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}