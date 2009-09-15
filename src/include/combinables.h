#ifndef COMBINABLES_H
#define COMBINABLES_H

#include <iostream>

namespace Combinables {

  class Brew {
  public:

    class recipe {
    public:
      bool operator<(const recipe& r2) const { return (this->bottle < r2.bottle) || (this->liquid < r2.liquid) || (this->herb < r2.herb); }
      int32_t bottle;
      int32_t liquid;
      int32_t herb;
    };

    Brew();
    ~Brew();
    void load(void);
    void save(void);
    void list(char_data *ch);
    int add(char_data *ch, char *argument);
    int remove(char_data *ch, char *argument);
    int size(void);
  private:
    static map<recipe, int32_t> recipes;
    struct loadError {};
    static const char RECIPES_FILENAME[];
    static bool initialized;
  };

  const char Brew::RECIPES_FILENAME[] = "brewables.dat";
  map<Brew::recipe, int> Brew::recipes;
  bool Brew::initialized = false;
}

#endif
