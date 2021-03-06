#ifndef __dubins_velocity_h__
#define __dubins_velocity_h__

#include <cfloat>
#include <iostream>
#include "dynamical_system.h"
using namespace std;

class dubins_velocity_optimization_data_c : public optimization_data_c
{
    public:
        int turning_radius;
        double accel;
        double T;
        bool is_initialized;
        dubins_velocity_optimization_data_c() : turning_radius(-1), is_initialized(false){}
        ~dubins_velocity_optimization_data_c(){}
};

class dubins_velocity_c : public dynamical_system_c<state_c<4>, control_c<2>, dubins_velocity_optimization_data_c >
{
    public:
        typedef typename dynamical_system_c::state_t state_t;
        typedef typename dynamical_system_c::control_t control_t;
        typedef dynamical_system_c::trajectory_t trajectory_t;
        typedef dubins_velocity_optimization_data_c dubins_velocity_optimization_data_t;

        double delta_distance;
        double accel_max;

#define num_turning_radii   (1)
#define num_velocities      (5)
        double turning_radii[num_turning_radii];
        double velocities[num_velocities];

        dubins_velocity_c()
        {
            delta_distance = 0.05;
            accel_max = 1.0;
            turning_radii[0] = 3;
            //turning_radii[1] = 6;
            //turning_radii[2] = 8;
#if 1
            velocities[0] = 0;
            velocities[1] = 0.1;
            velocities[2] = 0.2;
            velocities[2] = 0.5;
            velocities[3] = 0.75;
#else
            velocities[0] = 0.5;
#endif
        };

        int get_plotter_state(const state_t& s, double* ps)
        {
            ps[0] = s[0];
            ps[1] = s[1];
            ps[2] = s[3];
            return 0;
        }

        int sample_state(double* center, double* size, double* s)
        {
            for(int i : range(0,3))
                s[i] = center[i] + (RANDF-0.5)*size[i];
            int p = RANDF*4.0; 
            s[3] = velocities[p];

            while(s[2] > M_PI)
                s[2] -= 2*M_PI;
            while(s[2] < -M_PI)
                s[2] += 2*M_PI;
            return 0;
        }

        int extend_to(const state_t& si, const state_t& sf, trajectory_t& traj, dubins_velocity_optimization_data_t& opt_data)
        {
            bool return_trajectory = true;
            if(opt_data.turning_radius < 0)
            {
                if(evaluate_extend_cost(si, sf, opt_data) < 0)
                    return 1;
            }
            int& best_turning_radius = opt_data.turning_radius;
            double accel = opt_data.accel;
            extend_dubins_all(si.x, sf.x, return_trajectory, traj, turning_radii[best_turning_radius], accel);
            return 0;
        }

        double evaluate_extend_cost(const state_t& si, const state_t& sf,
                dubins_velocity_optimization_data_t& opt_data)
        {
            trajectory_t traj;
            if(opt_data.is_initialized)
            {
                double tr = turning_radii[opt_data.turning_radius];
                return extend_dubins_all(si.x, sf.x, false, traj, tr, opt_data.accel);
            }
            else
            {
                double min_cost = FLT_MAX;
                double dummy_accel;

                int& best_turning_radius = opt_data.turning_radius;
                best_turning_radius = -1;

                bool return_trajectory = false;
                for(int i=num_turning_radii-1; i >=0; i--)
                {
                    double tr = turning_radii[i];
                    double cost = extend_dubins_all(si.x, sf.x, return_trajectory, traj, tr, dummy_accel);
                    if(cost > 0)
                    {
                        if(cost < min_cost)
                        {
                            min_cost = cost;
                            best_turning_radius = i;
                            opt_data.T = min_cost;
                        }
                    }
                }
                if((min_cost < 0) || (min_cost > FLT_MAX/2.0))
                    return -1;

                if(opt_data.T*accel_max > fabs(sf.x[3] - si.x[3]))
                {
                    opt_data.accel = (sf[3] - si[3])/opt_data.T;
                    opt_data.is_initialized = true;
                }
                else
                    return -1;

                return min_cost;
            }
        }

        double extend_dubins_spheres(const double si[4], const double sf[4], int comb_no, double turning_radius, double accel,
                bool return_trajectory, trajectory_t& traj)
        {
            double x_s1 = si[0], x_s2 = sf[0];
            double y_s1 = si[1], y_s2 = sf[1];
            double t_s1 = si[2], t_s2 = sf[2];
            double v_s1 = si[3], v_s2 = sf[3];

            double x_tr = x_s2 - x_s1;
            double y_tr = y_s2 - y_s1;
            double t_tr = atan2 (y_tr, x_tr);

            double distance = sqrt (x_tr*x_tr + y_tr*y_tr);

            double x_start;
            double y_start;
            double t_start = 0;
            double x_end;
            double y_end;
            double t_end = 0;

            if (distance > 2 * turning_radius) 
            {  
                // disks do not intersect
                double t_balls = acos (2 * turning_radius / distance);
                switch (comb_no) 
                {
                    case 1:
                        t_start = t_tr - t_balls;
                        t_end = t_tr + M_PI - t_balls;
                        break;
                    case 2:
                        t_start = t_tr + t_balls;
                        t_end = t_tr - M_PI + t_balls;
                        break;
                    case 3:
                        t_start = t_tr - M_PI_2;
                        t_end = t_tr - M_PI_2;
                        break;
                    case 4:
                        t_start = t_tr + M_PI_2;
                        t_end = t_tr + M_PI_2;
                        break;
                    default:
                        return -1.0;
                }
            }
            else 
            { 
                // disks are intersecting
                switch (comb_no) 
                {
                    case 1:
                    case 2:
                        // No solution
                        return -1.0;
                        break;

                    case 3:
                        t_start = t_tr - M_PI_2;
                        t_end = t_tr - M_PI_2;
                        break;
                    case 4:
                        t_start = t_tr + M_PI_2;
                        t_end = t_tr + M_PI_2;
                        break;
                }
            }

            x_start = x_s1 + turning_radius * cos (t_start);
            y_start = y_s1 + turning_radius * sin (t_start);
            x_end = x_s2 + turning_radius * cos (t_end);
            y_end = y_s2 + turning_radius * sin (t_end);

            int direction_s1 = 1;
            if ( (comb_no == 2) || (comb_no == 4) ) {
                direction_s1 = -1;
            }
            int direction_s2 = 1;
            if ( (comb_no == 1) || (comb_no == 4) ) {
                direction_s2 = -1;
            }

            double t_increment_s1 = direction_s1 * (t_start - t_s1);
            double t_increment_s2 = direction_s2 * (t_s2 - t_end);
            modulo_zero_2pi(t_increment_s1);
            modulo_zero_2pi(t_increment_s2);

            if  ( ( (t_increment_s1 > M_PI) && (t_increment_s2 > M_PI) ) 
                    || ( (t_increment_s1 > 3*M_PI_2) || (t_increment_s2 > 3*M_PI_2) )  ){
                return -1.0;
            }

            // different costs
            double ts1c = t_increment_s1;
            double ts2c = t_increment_s2;
            modulo_mpi_pi(ts1c);
            modulo_mpi_pi(ts2c);

            // comment turning_cost for testing
            double total_cost = (( fabs(ts1c) + fabs(ts2c)) * turning_radius  + distance);

            if((v_s2*v_s2 - v_s1*v_s1) > 2.0*accel_max*total_cost)
                return -1.0;

            if (return_trajectory) 
            {
                traj.clear();
                traj.total_variation = total_cost;
                traj.dt = delta_distance;

                // Generate states/inputs
                double del_d = delta_distance;
                double del_t = del_d/turning_radius;

                double t_inc_curr = 0.0;
                double integration_time = 0;
                double state_curr[4] = {0};

                while (t_inc_curr < t_increment_s1) 
                {
                    double t_inc_rel = del_t;
                    t_inc_curr += del_t;
                    if (t_inc_curr > t_increment_s1) 
                    {
                        t_inc_rel -= t_inc_curr - t_increment_s1;
                        t_inc_curr = t_increment_s1;
                    }
                    integration_time += t_inc_rel*turning_radius;

                    state_curr[0] = x_s1 + turning_radius * cos (direction_s1 * t_inc_curr + t_s1);
                    state_curr[1] = y_s1 + turning_radius * sin (direction_s1 * t_inc_curr + t_s1);
                    state_curr[2] = direction_s1 * t_inc_curr + t_s1 + ( (direction_s1 == 1) ? M_PI_2 : 3.0*M_PI_2);
                    state_curr[3] = v_s1 + accel*integration_time;

                    const double control_curr = direction_s1*turning_radius;

                    modulo_mpi_pi(state_curr[2]);

                    traj.states.push_back(state_t(state_curr));
                    traj.controls.push_back(control_t(&control_curr));
                }

                double d_inc_curr = 0.0;
                while (d_inc_curr < distance) 
                {
                    double d_inc_rel = del_d;
                    d_inc_curr += del_d;
                    if (d_inc_curr > distance) {
                        d_inc_rel -= d_inc_curr - distance;
                        d_inc_curr = distance;
                    }
                    integration_time += d_inc_rel;

                    state_curr[0] = (x_end - x_start) * d_inc_curr / distance + x_start; 
                    state_curr[1] = (y_end - y_start) * d_inc_curr / distance + y_start; 
                    state_curr[2] = direction_s1 * t_inc_curr + t_s1 + ( (direction_s1 == 1) ? M_PI_2 : 3.0*M_PI_2);
                    state_curr[3] = v_s1 + accel*integration_time;

                    const double control_curr = 0;

                    modulo_mpi_pi(state_curr[2]);

                    traj.states.push_back(state_t(state_curr));
                    traj.controls.push_back(control_t(&control_curr));
                }

                t_inc_curr = 0.0;
                while (t_inc_curr < t_increment_s2) 
                {
                    double t_inc_rel = del_t;
                    t_inc_curr += del_t;
                    if (t_inc_curr > t_increment_s2)  {
                        t_inc_rel -= t_inc_curr - t_increment_s2;
                        t_inc_curr = t_increment_s2;
                    }
                    integration_time += t_inc_rel*turning_radius;

                    state_curr[0] = x_s2 + turning_radius * cos (direction_s2 * (t_inc_curr - t_increment_s2) + t_s2);
                    state_curr[1] = y_s2 + turning_radius * sin (direction_s2 * (t_inc_curr - t_increment_s2) + t_s2);
                    state_curr[2] = direction_s2 * (t_inc_curr - t_increment_s2) + t_s2 
                        + ( (direction_s2 == 1) ?  M_PI_2 : 3.0*M_PI_2 );
                    state_curr[3] = v_s1 + accel*integration_time;

                    const double control_curr = direction_s2*turning_radius;

                    modulo_mpi_pi(state_curr[2]);

                    traj.states.push_back(state_t(state_curr));
                    traj.controls.push_back(control_t(&control_curr));
                }
            }
            return total_cost;
        }

        double extend_dubins_all(const double si[4], const double sf[4], bool return_trajectory,
                trajectory_t& traj, double turning_radius, double accel)
        {
            double ti = si[2];
            double tf = sf[2];
            double sin_ti = sin (-ti);
            double cos_ti = cos (-ti);
            double sin_tf = sin (-tf);
            double cos_tf = cos (-tf);

            double si_left[4] = {
                si[0] + turning_radius * sin_ti,
                si[1] + turning_radius * cos_ti,
                ti + (double)(3 * M_PI_2),
                si[3]
            };
            double si_right[4] = {
                si[0] - turning_radius * sin_ti,
                si[1] - turning_radius * cos_ti,
                ti + (double)M_PI_2,
                si[3]
            };
            double sf_left[4] = {
                sf[0] + turning_radius * sin_tf,
                sf[1] + turning_radius * cos_tf,
                tf + (double)(3 * M_PI_2),
                sf[3]
            };
            double sf_right[4] = {
                sf[0] - turning_radius * sin_tf,
                sf[1] - turning_radius * cos_tf,
                tf + (double)M_PI_2,
                sf[3]
            };

            // 2. extend all four spheres
            double times[4]; 
            times[0] = extend_dubins_spheres (si_left, sf_right, 1, turning_radius, accel,
                    return_trajectory, traj);
            times[1] = extend_dubins_spheres (si_right, sf_left, 2, turning_radius, accel,
                    return_trajectory, traj);
            times[2] = extend_dubins_spheres (si_left, sf_left, 3, turning_radius, accel,
                    return_trajectory, traj);
            times[3] = extend_dubins_spheres (si_right, sf_right, 4, turning_radius, accel,
                    return_trajectory, traj);

            double min_time = FLT_MAX/2;
            int comb_min = -1;
            for (int i = 0; i < 4; i++) 
            {
                if  ( (times[i] >= 0.0) && (times[i] < min_time) ) 
                {
                    comb_min = i+1;
                    min_time = times[i];
                }
            }

            if (comb_min == -1)
                return -1.0;

            return min_time;
        }

        void test_extend_to()
        {
            trajectory_t traj;
            double zero[4] ={0};
            state_t origin(zero);
            double goal[4] = {10, 2, M_PI/4, 1};
            state_t sr(goal);
            sr.print(cout, "sampled:","\n");

            dubins_velocity_optimization_data_c opt_data;
            extend_to(origin, sr, traj, opt_data);
            cout<<"cost: "<< traj.total_variation<<endl;
            traj.print();
        }
};

#endif
