#ifndef RW_LOCK_HPP
#define RW_LOCK_HPP

#include <boost/thread.hpp>

//there is no standard way to use rw lock in c++11
//so use boost::shared_mutex to implement it

typedef boost::shared_mutex rw_mutex_t;
typedef boost::shared_lock<boost::shared_mutex> rlock_t;
typedef boost::unique_lock<boost::shared_mutex> wlock_t;

#endif // RW_LOCK_HPP
