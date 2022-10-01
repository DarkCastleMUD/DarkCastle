#ifdef USE_SQL
#ifndef BACKEND_H_
#define BACKEND_H_

class Backend {
 private:

 protected:

 public:
  Backend();
  virtual void save(char_data *ch, char_file_u4 *st) = 0;
  
  virtual char_data *load(void) = 0;
  virtual ~Backend();
};

#endif
#endif
