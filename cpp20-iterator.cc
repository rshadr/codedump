#include <cstddef>
#include <cstdio>
#include <iterator>
#include <algorithm>
#include <ranges>
#include <array>


class MyClass final {
  public:
    MyClass(int n_val)
    {
      this->n_.fill(n_val);
    }
    ~MyClass() { };


  public:
    struct iterator {
      using difference_type   = std::ptrdiff_t;
      using value_type        = int;
      using pointer           = int*;
      using reference         = int;
      using iterator_category = std::bidirectional_iterator_tag;

      iterator(int *startp_,
               int *endp_,
               int *curp_)
      {
        this->startp_ = startp_;
        this->endp_   = endp_;
        this->curp_   = curp_;
      }

      value_type& operator*() const
      {
        return *this->curp_;
      }


      iterator& operator++()
      {
        this->curp_++;
        return *this;
      }

      iterator operator++(int)
      {
        auto tmp = *this;
        ++*this;
        return tmp;
      }


      iterator& operator--()
      {
        this->curp_--;
        return *this;
      }

      iterator operator--(int)
      {
        auto tmp = *this;
        --*this;
        return tmp;
      }


      bool operator==(const iterator& it2)
      {
        return (this->startp_ == it2.startp_
             && this->endp_   == it2.endp_
             && this->curp_   == it2.curp_);
      }

      bool operator!=(const iterator& it2)
      {
        return !(*this == it2);
      }
      

      private:
        int *startp_;
        int *endp_;
        int *curp_;
    };

    iterator begin(void)
    {
      int *n_data = n_.data();
      return iterator(&n_data[0],
                      &n_data[n_.size()],
                      &n_data[0]);
    }

    iterator end(void)
    {
      int *n_data = n_.data();
      return iterator(&n_data[0],
                      &n_data[n_.size()],
                      &n_data[n_.size()]);
    }

  private:
    std::array< int, 256> n_;
};


int
main([[maybe_unused]] int argc,
     [[maybe_unused]] char *argv[])
{
  MyClass mc(78);

  int begin = 0;

  for (const auto& i : mc) {
    std::printf("%d\n", i);
  }

  return 0;
}

