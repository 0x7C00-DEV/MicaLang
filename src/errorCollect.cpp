#include "../include/errorCollect.h"


ErrorCollect::ErrorCollect(OnErrorTrig* trig) {
    this->onError = trig;
}

void ErrorCollect::emitError(Error* error, bool istrig) {
    this->errors.push_back(error);
    if (istrig) this->onError(error);
}

void ErrorCollect::save() {
    saveMarks.push_back(errors.size());
}

void ErrorCollect::restore() {
    if (saveMarks.empty()) return;
    size_t mark = saveMarks.back();
    saveMarks.pop_back();
    while (errors.size() > mark) {
        delete errors.back();
        errors.pop_back();
    }
}