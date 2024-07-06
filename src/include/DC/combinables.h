#ifndef COMBINABLES_H
#define COMBINABLES_H

#include <iostream>

namespace Combinables
{

  class Brew
  {
  public:
    class recipe
    {
    public:
      bool operator<(const recipe &r2) const
      {
        if (container < r2.container)
        {
          return true;
        }
        else if (container == r2.container)
        {
          if (liquid < r2.liquid)
          {
            return true;
          }
          else if (liquid == r2.liquid)
          {
            if (herb < r2.herb)
            {
              return true;
            }
          }
        }

        return false;
      }
      vnum_t herb = {};
      int64_t liquid = {};
      vnum_t container = {};
    };

    Brew();
    ~Brew();
    void load(void);
    void save(void);
    void list(Character *ch);
    int add(Character *ch, char *argument);
    int remove(Character *ch, char *argument);
    int size(void);
    int find(recipe);

  private:
    static std::map<recipe, int32_t> recipes;
    struct loadError
    {
    };
    static const char RECIPES_FILENAME[];
    static bool initialized;
  };

  const char Brew::RECIPES_FILENAME[] = "brewables.dat";
  std::map<Brew::recipe, int> Brew::recipes;
  bool Brew::initialized = false;

  // I feel just wrong doing this, but it's the easiest way at the moment
  // this really should be combined into a parent class with 2 children
  // inheriting common functionality...

  class Scribe
  {
  public:
    class recipe
    {
    public:
      bool operator<(const recipe &r2) const
      {
        if (ink < r2.ink)
        {
          return true;
        }
        else if (ink == r2.ink)
        {
          if (dust < r2.dust)
          {
            return true;
          }
          else if (dust == r2.dust)
          {
            if (pen < r2.pen)
            {
              return true;
            }
            else if (pen == r2.pen)
            {
              if (paper < r2.paper)
              {
                return true;
              }
            }
          }
        }

        return false;
      }
      vnum_t ink;
      vnum_t dust;
      vnum_t pen;
      vnum_t paper;
    };

    Scribe();
    ~Scribe();
    void load(void);
    void save(void);
    void list(Character *ch);
    int add(Character *ch, char *argument);
    int remove(Character *ch, char *argument);
    int size(void);
    int find(recipe);

  private:
    static std::map<recipe, int32_t> recipes;
    struct loadError
    {
    };
    static const char RECIPES_FILENAME[];
    static bool initialized;
  };

  const char Scribe::RECIPES_FILENAME[] = "scribe.dat";
  std::map<Scribe::recipe, int> Scribe::recipes;
  bool Scribe::initialized = false;
}

#endif
