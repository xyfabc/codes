#ifndef UNCOPYABLE_H
#define UNCOPYABLE_H

class Uncopyable
{
private:
    Uncopyable(const Uncopyable &);
    Uncopyable& operator= (const Uncopyable &);    

protected:
    Uncopyable() {};
    ~Uncopyable() {};
};

#endif /* UNCOPYABLE_H */
