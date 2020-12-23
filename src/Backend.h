#ifdef USE_SQL
#ifndef BACKEND_H_
#define BACKEND_H_

class Backend {
 private:

 protected:

 public:
  Backend();
  virtual void save(CHAR_DATA *ch, char_file_u *st) = 0;
  
  virtual CHAR_DATA *load(void) = 0;
  virtual ~Backend();
};

#endif
#endif
