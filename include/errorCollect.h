#ifndef ERROR_COLLECT
#define ERROR_COLLECT
#include "errors.h"
#include <vector>
using OnErrorTrig = void(Error*);

class ErrorCollect {
public:
    std::vector<Error*> errors;
    ErrorCollect(OnErrorTrig*);
    void emitError(Error*, bool);
    void save();
    void restore();
private:
    OnErrorTrig* onError;
    std::vector<size_t> saveMarks;
};

#endif