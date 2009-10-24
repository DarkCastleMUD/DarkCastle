#ifndef COMBINABLES_H
#define COMBINABLES_H

#include <iostream>

namespace Combinables {

  class Brew {
  public:

    class recipe {
    public:
      bool operator<(const recipe& r2) const { 
	if (container < r2.container) {
	  return true;
	} else if (container == r2.container) {
	  if (liquid < r2.liquid) {
	    return true;
	  } else if (liquid == r2.liquid) {
	    if (herb < r2.herb) {
	      return true;
	    }
	  }
	}
	  
	return false;	      
      }
      int32_t herb;
      int32_t liquid;
      int32_t container;
    };

    Brew();
    ~Brew();
    void load(void);
    void save(void);
    void list(char_data *ch);
    int add(char_data *ch, char *argument);
    int remove(char_data *ch, char *argument);
    int size(void);
    int find(recipe);
  private:
    static map<recipe, int32_t> recipes;
    struct loadError {};
    static const char RECIPES_FILENAME[];
    static bool initialized;
  };

  const char Brew::RECIPES_FILENAME[] = "brewables.dat";
  map<Brew::recipe, int> Brew::recipes;
  bool Brew::initialized = false;

  // I feel just wrong doing this, but it's the easiest way at the moment
  // this really should be combined into a parent class with 2 children
  // inheriting common functionality...

  class Scribe {
  public:

    class recipe {
    public:
      bool operator<(const recipe& r2) const { 
	if (container < r2.container) {
	  return true;
	} else if (container == r2.container) {
	  if (liquid < r2.liquid) {
	    return true;
	  } else if (liquid == r2.liquid) {
	    if (herb < r2.herb) {
	      return true;
	    }
	  }
	}
	  
	return false;	      
      }
      int32_t ink;
      int32_t dust;
      int32_t pen;
      int32_t paper;
    };

    Scribe();
    ~Scribe();
    void load(void);
    void save(void);
    void list(char_data *ch);
    int add(char_data *ch, char *argument);
    int remove(char_data *ch, char *argument);
    int size(void);
    int find(recipe);
  private:
    static map<recipe, int32_t> recipes;
    struct loadError {};
    static const char RECIPES_FILENAME[];
    static bool initialized;
  };

  const char Scribe::RECIPES_FILENAME[] = "scribe.dat";
  map<Scribe::recipe, int> Scribe::recipes;
  bool Scribe::initialized = false;
}

#endif
