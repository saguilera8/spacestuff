#define _CRT_SECURE_NO_WARNINGS

#include <cmath>
#include <cstdio>
#include <map>

#include "solution.h"
#include "test_util.h"

using User = uint32_t;
using Sat = uint32_t;
using Color = char;

using namespace std::chrono_literals;

static constexpr float kPi = 3.14159265f;

static constexpr Color kColors[] = {'A', 'B', 'C', 'D'};

static std::istream& operator>>(std::istream& is, Vector3& vector)
{
    return is >> vector.x >> vector.y >> vector.z;
}

static float dot(const Vector3& a, const Vector3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static Vector3 operator-(const Vector3& a, const Vector3& b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

static float length(const Vector3& v)
{
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static Vector3 normalize(const Vector3& v)
{
    const float m = length(v);
    return {v.x / m, v.y / m, v.z / m};
}

static bool check_color(Color color)
{
    for (const Color c : kColors)
    {
        if (color == c)
        {
            return true;
        }
    }
    return false;
}

int main(int argc, char** argv)
{
    CHECK(argc == 3, "USAGE: %s OUT_PATH TEST_CASE", argv[0]);

    // Parse test case.
    float min_coverage = 1.0;
    bool bonus = false;
    std::vector<Vector3> users;
    std::vector<Vector3> sats;
    for (const auto& [line_number, line] : read_file(argv[2]))
    {
        std::istringstream tokens(line, std::istringstream::in);
        int id = 0;
        Vector3 pos{};
        std::string token;
        tokens >> token;
        if (token == "min_coverage")
        {
            tokens >> min_coverage;
        }
        else if (token == "bonus")
        {
            tokens >> bonus;
        }
        else
        {
            tokens >> id >> pos;
            if (token == "sat")
            {
                sats.push_back(pos);
            }
            else if (token == "user")
            {
                users.push_back(pos);
            }
            else
            {
                FAIL("Invalid token '%s'.", token.c_str());
            }
        }
    }
    printf(
        GRAY "Scenario: " RESET "%.2f%% coverage (%zu users, %zu sats)" RESET "\n",
        static_cast<double>(100 * min_coverage),
        users.size(),
        sats.size());

    // Delete old solution file, if any.
    if (remove("solution.txt") != 0)
    {
        CHECK(errno == ENOENT, "Unable to delete existing solution.txt file.");
    }

    // Make copies of sats and users to pass to candidate code.
    std::vector<Vector3> users_copy = users;
    std::vector<Vector3> sats_copy = sats;

    // Run candidate code.
    const auto start = std::chrono::high_resolution_clock::now();
    solve(
        users_copy.data(),
        static_cast<int>(users.size()),
        sats_copy.data(),
        static_cast<int>(sats.size()));
    const auto time = std::chrono::high_resolution_clock::now() - start;
    std::vector<std::tuple<User, Sat, Color>> solution;
    {
        {
            FILE* fp = fopen("solution.txt", "rb");
            CHECK(fp, "solution.txt file not found.");
            if (fp)
            {
                fclose(fp);
            }
        }
        for (const auto& [line_number, line] : read_file("solution.txt"))
        {
            if (line.empty() || line[0] == '#')
                continue;
            std::istringstream tokens(line, std::istringstream::in);
            std::string s;
            size_t n;

            // User.
            s.clear();
            tokens >> s;
            const uint32_t user = uint32_t(std::stoi(s, &n, 10));
            CHECK(
                !s.empty() && n == s.size() && user < users.size(),
                "Invalid user field reading solution line %d (%s)",
                static_cast<int>(line_number),
                line.c_str());

            // Sat.
            s.clear();
            tokens >> s;
            const uint32_t sat = uint32_t(std::stoi(s, &n, 10));
            CHECK(
                !s.empty() && n == s.size() && sat < sats.size(),
                "Invalid sat field reading solution line %d (%s)",
                static_cast<int>(line_number),
                line.c_str());

            // Color.
            s.clear();
            tokens >> s;
            CHECK(
                s.size() == 1 && check_color(s[0]),
                "Invalid color field reading solution line %d (%s)",
                static_cast<int>(line_number),
                line.c_str());
            const char color = s[0];

            CHECK(
                tokens.eof(),
                "Expected end of line after color field reading solution line %d (%s)",
                static_cast<int>(line_number),
                line.c_str());

            solution.emplace_back(user, sat, color);
        }
    }

    // Check solution.
    float total_dist = 0.0f;
    std::vector<bool> served_users(users.size());
    for (const auto& p : solution)
    {
        const User user = std::get<0>(p);
        const Sat sat = std::get<1>(p);
        const Vector3 user_pos = users[user];
        const Vector3 sat_pos = sats[sat];
        total_dist += length(sat_pos - user_pos);
        CHECK(!served_users[user], "User %d served more than once", user);
        served_users[user] = true;
    }

    std::map<Sat, std::vector<std::pair<Color, User>>> beams;
    for (const auto& it : solution)
    {
        const User user = std::get<0>(it);
        const Sat sat = std::get<1>(it);
        const Color color = std::get<2>(it);
        const Vector3 user_pos = users[user];
        const Vector3 sat_pos = sats[sat];
        const float angle =
            180.0f * acosf(dot(normalize(user_pos), normalize(sat_pos - user_pos))) / kPi;
        CHECK(
            angle <= 45,
            "User %d cannot see satellite %d (%.2f degrees from vertical)",
            user,
            sat,
            static_cast<double>(angle));

        beams[sat].emplace_back(color, user);
    }
    for (const auto& it : beams)
    {
        const Sat sat = it.first;
        const Vector3 sat_pos = sats.at(sat);
        const size_t sat_beams = it.second.size();
        CHECK(
            sat_beams <= 32,
            "Satellite %d cannot serve more than 32 users (%zu assigned)",
            sat,
            sat_beams);

        for (uint64_t i1 = 0; i1 < it.second.size(); ++i1)
        {
            const std::pair<Color, User>& u1 = it.second[i1];
            const Color color1 = u1.first;
            const User user1 = u1.second;

            for (uint64_t i2 = 0; i2 < it.second.size(); ++i2)
            {
                if (i1 == i2)
                    continue;
                const std::pair<Color, User>& u2 = it.second[i2];

                const Color color2 = u2.first;
                const User user2 = u2.second;

                if (color1 == color2)
                {
                    const Vector3 user1_pos = users.at(user1);
                    const Vector3 user2_pos = users.at(user2);
                    const float angle =
                        180.0f *
                        acosf(dot(normalize(user1_pos - sat_pos), normalize(user2_pos - sat_pos))) /
                        kPi;
                    CHECK(
                        angle >= 10.0f,
                        "Users %d and %d on satellite %d color %c are too close (%.2f degrees)",
                        user1,
                        user2,
                        sat,
                        color1,
                        static_cast<double>(angle));
                }
            }
        }
    }

    constexpr float kUsecPerKm = 3.33564095f;
    const float avg_latency =
        solution.empty() ? 0.0f
                         : 2.0f * kUsecPerKm / static_cast<float>(solution.size()) * total_dist;
    const float duration_s =
        static_cast<float>(std::chrono::duration_cast<std::chrono::nanoseconds>(time).count()) *
        1e-9f;
    const float coverage = static_cast<float>(solution.size()) / static_cast<float>(users.size());

    printf(
        GRAY "Solution: " RESET BOLD "%s%.2f%%" RESET
             " coverage (%zu users) and %.0fus latency in %s" BOLD "%.2fs" RESET "\n",
        coverage >= min_coverage ? GREEN : (bonus ? YELLOW : RED),
        static_cast<double>(100.0f * coverage),
        solution.size(),
        static_cast<double>(avg_latency),
        time > 60s       ? RED
        : time > 60s / 2 ? YELLOW
                         : GREEN,
        static_cast<double>(duration_s));

    if (bonus && coverage < min_coverage)
    {
        printf(BOLD YELLOW "BONUS CASE FAIL: Too few users served" RESET "\n");
    }
    else
    {
        CHECK(coverage >= min_coverage, "Too few users served");
    }

    // Output stats.
    FILE* out = fopen(argv[1], "a");
    CHECK(out, "Error opening output %s: %s (errno=%d)", argv[1], strerror(errno), errno);
    fprintf(
        out,
        "%-44s %6.2f%% %6.0fus %6.2fs\n",
        argv[2],
        static_cast<double>(100.0f * coverage),
        static_cast<double>(avg_latency),
        static_cast<double>(duration_s));
}
