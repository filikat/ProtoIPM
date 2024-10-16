#ifndef IPM_CONST_H
#define IPM_CONST_H

enum OptionNla {
  kOptionNlaMin = 0,
  kOptionNlaAugmented = kOptionNlaMin,
  kOptionNlaNormEq,
  kOptionNlaMax = kOptionNlaNormEq,
  kOptionNlaDefault = kOptionNlaNormEq
};

enum OptionFact {
  kOptionFactMin = 0,
  kOptionFactChol = kOptionNlaMin,
  kOptionFactLDLt,
  kOptionFactMax = kOptionFactLDLt,
  kOptionFactDefault = kOptionFactLDLt,
};

enum OptionFormat {
  kOptionFormatMin = 0,
  kOptionFormatFull = kOptionFormatMin,
  kOptionFormatHybridPacked,
  kOptionFormatHybridHybrid,
  kOptionFormatMax = kOptionFormatHybridHybrid,
  kOptionFormatDefault = kOptionFormatHybridPacked
};

struct Options {
  int nla = kOptionNlaDefault;
  int fact = kOptionFactDefault;
  int format = kOptionFormatDefault;
  int verbose = false;
};

const int kMaxIterations = 100;
const double kIpmTolerance = 1e-8;

const double kInteriorScaling = 0.999;

#endif