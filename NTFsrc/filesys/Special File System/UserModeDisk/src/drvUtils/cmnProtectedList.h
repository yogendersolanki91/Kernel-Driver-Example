// cmnprotectedlist.h
#ifndef CMN_PROTECTED_LIST_H
#define CMN_PROTECTED_LIST_H
#include "auto_resource.h"
#include "list"
namespace utils
{

template<class T, class Sync, class Event>
class ProtectedList
{
    std::list<T> list_;
    mutable Sync guard_;
    Event ownNotEmptyEvent_;
    Event& notEmptyEvent_;
    ProtectedList(const ProtectedList& );
    const ProtectedList& operator=(const ProtectedList& );

protected:
    virtual std::list<T> & get_list() { return list_; }
    virtual const std::list<T> & get_list() const { return list_; }
    virtual Sync & get_lock() const { return guard_; }
    virtual void on_pop(){}
public:
    ProtectedList()
        : notEmptyEvent_(ownNotEmptyEvent_)
    {
    }
    ProtectedList(Event& notEmptyEvent)
        : notEmptyEvent_(notEmptyEvent)
    {
    }
    virtual ~ProtectedList()
    {
    }
    
    Event * get_event_ptr()
    {
        return &notEmptyEvent_;
    }
    
    void push(const T& val)
    {
        AutoResource<Sync> guard(guard_);
        list_.push_front(val);
        if(list_.size() == 1)
            notEmptyEvent_.set();
    }
    void push_back(const T& val)
    {
        AutoResource<Sync> guard(guard_);
        list_.push_back(val);
        if(list_.size() == 1)
            notEmptyEvent_.set();
    }
    bool pop(T& val)
    {
        AutoResource<Sync> guard(guard_);
        if(list_.empty())
            return false;
        val = list_.front();
        list_.pop_front();
        if(list_.empty())
            notEmptyEvent_.reset();
        on_pop();
        return true;
    }

    bool pop_swap(T& val)
    {
        AutoResource<Sync> guard(guard_);
        if(list_.empty())
            return false;
        
        list_.front().swap(val);
        list_.pop_front();

        if(list_.empty())
            notEmptyEvent_.reset();
        
        on_pop();
        return true;
    }

    size_t size()
    {
        AutoResource<Sync> guard(guard_);
        return list_.size();
    }

    void clear()
    {
        AutoResource<Sync> guard(guard_);
        list_.clear();
    }
};

}//namespace utils
#endif