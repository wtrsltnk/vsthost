#ifndef COMMON_H
#define COMMON_H

#define ASSERT_THROW(c,e)   if(!(c)) { throw std::runtime_error(e); }
#define CLOSE_HANDLE(x)     if((x)) { CloseHandle(x); x = nullptr; }
#define RELEASE(x)          if((x)) { (x)->Release(); x = nullptr; }

#endif // COMMON_H
