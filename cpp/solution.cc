#include "solution.h"

namespace my_sol
{

    const int BEAMS = 32; // Beams per sat
    const int COLORS = 4; // Beam Colors

    const float COS_45 = std::cos(M_PI / 4.0);        // 45°
    const float COS_10 = std::cos(10.0 * M_PI / 180); // 10°

    /* Implemented my own Vector3d
       it has all the math functionality I need it.
    */
    struct Vector3d
    {
        float x, y, z;

        Vector3d(float x, float y, float z) : x(x), y(y), z(z) {}

        // subtract two vectors
        Vector3d operator-(const Vector3d &other) const
        {
            return Vector3d(x - other.x, y - other.y, z - other.z);
        }

        // get vector magnitude
        float magnitude() const
        {
            return std::sqrt(x * x + y * y + z * z);
        }

        // unit vector
        Vector3d unit() const
        {
            float mag = magnitude();
            if (mag == 0)
                return Vector3d(0, 0, 0);
            return Vector3d(x / mag, y / mag, z / mag);
        }

        // get dot product
        float dot(const Vector3d &other) const
        {
            return x * other.x + y * other.y + z * other.z;
        }
    };

    // Represent a User and its pos
    struct User
    {
        int id;
        Vector3d position;
        User() : id(-1), position(0, 0, 0) {}
        User(int id, const Vector3d &pos) : id(id), position(pos) {}
    };

    // Represent a beam, pos, and if its assigned
    struct Beam
    {
        bool assigned;
        int user_id;
        Vector3d dir; // always unit

        Beam() : assigned(false), user_id(-1), dir(0, 0, 0) {}
        Beam(int uid, const Vector3d &v) : assigned(false), user_id(uid), dir(v.unit()) {}
    };

    // Represtne the satellites beams
    struct SatBeams
    {
        std::vector<std::vector<Beam>> colors; // colors[color_index][beam_index]

        SatBeams()
        {
            colors.resize(COLORS);
            for (auto &vec : colors)
            {
                vec.resize(BEAMS / COLORS, Beam());
            }
        }

        // functions tries to assign a user to satellite
        bool assign(int user_id, const Vector3d &dir)
        {
            // make sure we dont exceed sat capacity
            int total_assigned = 0;
            for (int color = 0; color < COLORS; ++color)
                for (auto &beam : colors[color])
                    if (beam.assigned)
                        total_assigned++;
            if (total_assigned >= BEAMS)
                return false;

            // beam to be assign to a sat
            Beam new_beam(user_id, dir);

            // greedily assign to open beams slot
            for (int color = 0; color < COLORS; color++)
            {
                for (auto &beam : colors[color])
                {
                    if (beam.assigned)
                        continue;

                    // check beam conflict within same conflict
                    if (!beam_conflict(color, new_beam))
                    {
                        beam = new_beam;
                        beam.assigned = true;
                        return true;
                    }
                }
            }

            // try rearranging beams
            for (int color = 0; color < COLORS; color++)
            {
                for (auto &beam : colors[color])
                {
                    if (!beam.assigned)
                        continue;
                    // new beam color
                    for (int new_color = 0; new_color < COLORS; new_color++)
                    {
                        if (new_color == color)
                            continue;
                        for (auto &slot : colors[new_color])
                        {
                            // if thsi slot is not take and no conflict, reassign
                            if (!slot.assigned && !beam_conflict(new_color, beam))
                            {

                                slot = beam;
                                slot.assigned = true;
                                beam.assigned = false;
                            }
                        }

                        // succesful relocation
                        if (!beam.assigned)
                            break;
                    }

                    if (!beam.assigned && !beam_conflict(color, new_beam))
                    {
                        beam = new_beam;
                        beam.assigned = true;
                        return true;
                    }
                }
            }

            return false;
        }

        bool beam_conflict(int color, const Beam &beam_a)
        {
            for (const auto &beam_b : colors[color])
            {
                if (!beam_b.assigned)
                    continue;
                if (beam_a.dir.dot(beam_b.dir) > COS_10)
                    return true;
            }
            return false;
        }
    };

    struct Sat
    {
        int id;
        Vector3d position;
        SatBeams beams;
        Sat() : id(-1), position(0, 0, 0), beams() {}
        Sat(int id, const Vector3d &pos) : id(id), position(pos), beams() {}
    };

    void get_viable_satellites(std::vector<std::vector<int>> &user_to_sats,
                               const std::vector<User> &users,
                               const std::vector<Sat> &sats)
    {
        size_t users_size = users.size(), sats_size = sats.size();

        for (int id_user = 0; id_user < users_size; id_user++)
        {
            const User &user = users[id_user];
            Vector3d user_up = user.position.unit();

            for (int id_sat = 0; id_sat < sats_size; id_sat++)
            {
                const Sat &sat = sats[id_sat];

                // vertical
                Vector3d beam = (sat.position - user.position).unit(); // beam to user
                float dot = beam.dot(user_up);

                // if user can connect
                if (dot >= COS_45)
                {
                    user_to_sats[id_user].push_back(id_sat);
                }
            }
        }
    }

    void sort_users_by_sat_count(std::vector<std::pair<int, int>> &user_options_count,
                                 const std::vector<std::vector<int>> &user_to_sats)
    {
        size_t size = user_to_sats.size();

        for (int user_id = 0; user_id < size; user_id++)
        {
            // users -> count
            // we need this ot sort
            user_options_count.push_back({user_id, (int)user_to_sats[user_id].size()});
        }

        std::sort(user_options_count.begin(), user_options_count.end(),
                  [](auto &a, auto &b)
                  { return a.second < b.second; });
    }

    void assign_users_to_sats(const std::vector<std::pair<int, int>> &user_options_count,
                              const std::vector<std::vector<int>> &user_to_sats,
                              const std::vector<User> &users,
                              std::vector<Sat> &sats)
    {
        for (const auto &[user_id, _] : user_options_count)
        {
            for (const auto &sat_id : user_to_sats[user_id])
            {
                Vector3d beam_dir = (users[user_id].position - sats[sat_id].position).unit();

                if (sats[sat_id].beams.assign(user_id, beam_dir))
                {
                    break;
                }
            }
        }
    }

    void print_solution(const std::vector<Sat> &sats)
    {
        std::ofstream out("solution.txt");

        size_t sats_size = sats.size();
        for (int sat = 0; sat < sats_size; sat++)
        {
            for (int color = 0; color < COLORS; color++)
            {
                for (auto &beam : sats[sat].beams.colors[color])
                {
                    if (beam.assigned)
                    {
                        out << beam.user_id << " "
                            << sat << " "
                            << char('A' + color) << "\n";
                    }
                }
            }
        }
    }

    void my_solution(const std::vector<User> &users, std::vector<Sat> &sats)
    {
        // map for satellites users can connect to
        std::vector<std::vector<int>> user_to_sats(users.size());

        // fill map of user -> list(sats) with sats they can connect
        get_viable_satellites(user_to_sats, users, sats);

        std::vector<std::pair<int, int>> user_options_count; // (user_id, count of satelites)

        // sort users in ascending order from sat count
        // THis is done to give priority to users
        //  with least satellite connections available
        sort_users_by_sat_count(user_options_count, user_to_sats);

        assign_users_to_sats(user_options_count, user_to_sats, users, sats);

        print_solution(sats);
    }
}

void solve(const Vector3 *users, const int num_users, const Vector3 *sats, const int num_sats)
{
    // TODO: Implement.
    // I understand that use of artificial intelligence (AI) on this assessment is prohibited.

    // INIT my vectors
    std::vector<my_sol::User> Users;
    Users.reserve(num_users);

    std::vector<my_sol::Sat> Sats;
    Sats.reserve(num_sats);

    for (int i = 0; i < num_users; i++)
    {
        Users.emplace_back(i, my_sol::Vector3d(users[i].x, users[i].y, users[i].z));
    }

    for (int i = 0; i < num_sats; i++)
    {
        Sats.emplace_back(i, my_sol::Vector3d(sats[i].x, sats[i].y, sats[i].z));
    }

    my_sol::my_solution(Users, Sats);
}