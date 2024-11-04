// Waveform.h
#ifndef WAVEFORM_H
#define WAVEFORM_H

class Waveform {
public:
    virtual ~Waveform() {}
    virtual void execute() = 0;  // Pure virtual function for running the waveform
};

#endif
