#ifndef FACTORY_H_
#define FACTORY_H_

#include <map>
#include <vector>

template<class Abstract, class KeyType, class ... Args>
class Factory
{
  public:
    Factory()
    { }

    struct Worker 
    {
      virtual Abstract * CreateInstance(Args ... args) = 0;
    };

    typedef std::map<KeyType, Worker *> WorkerMap;

    template <class Concrete>
    struct ConcreteWorker : public Worker
    {
      Abstract * CreateInstance(Args ... args) override
      { return new Concrete(args...); }
    };

    template <class Concrete>
    void AddConcreteClass(const KeyType & key)
    { m_workers[key] = new ConcreteWorker<Concrete>(); }

    Abstract * CreateInstance(const KeyType & key, Args ... args)
    { 
      typename WorkerMap::iterator r = m_workers.find(key);
      if (r == m_workers.end())
        return NULL;
      return r->second->CreateInstance(args...);
    }

    bool HasKey(const std::string & key) const
    {
      return m_workers.count(key) != 0;
    }

    size_t GetKeys(std::vector<KeyType> & keys)
    {
      keys.clear();
      for (auto & r : m_workers)
        keys.push_back(r.first);
      return keys.size();  
    }

  protected:  
    WorkerMap m_workers;
};

#endif  // FACTORY_H_