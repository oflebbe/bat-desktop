#ifndef CLASSIFY_H
#define CLASSIFY_H

#include <string>
#include <fftw3.h>

class Classifier
{
private:
    bool relevant;
    QImage qimage;
    int SIZE;
    float *in;
    fftwf_complex *out;
    fftwf_plan plan;
    std::vector<float> window;

public:
    Classifier(int SIZE);
    void open(const std::string &filename);

    inline bool isRelevant() { return relevant; }
    inline QImage image() const { return qimage; }
    ~Classifier();
};

#endif