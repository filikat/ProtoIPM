#ifndef IPM_CONST_H
#define IPM_CONST_H

enum OptionNla {
  kOptionNlaMin = 0,
  kOptionNlaAugmented = kOptionNlaMin,
  kOptionNlaNormEq,
  kOptionNlaMax = kOptionNlaNormEq,
  kOptionNlaDefault = kOptionNlaNormEq
};

enum OptionFormat{
  kOptionFormatMin = 0,
  kOptionFormatFull = kOptionFormatMin,
  kOptionFormatHybridPacked,
  kOptionFormatHybridHybrid,
  kOptionFormatMax = kOptionFormatHybridHybrid,
  kOptionFormatDefault = kOptionFormatHybridPacked
};

enum OptionPredCor {
  kOptionPredcorMin = 0,
  kOptionPredcorOff = kOptionPredcorMin,
  kOptionPredcorOn = 1,
  kOptionPredcorMax = kOptionPredcorOn,
  kOptionPredcorDefault = kOptionPredcorOn
};

const int kMaxIterations = 100;
const double kIpmTolerance = 1e-8;

const double kSigmaInitial = 0.5;
const double kSigmaMin = 0.05;
const double kSigmaMax = 0.95;
const double kInteriorScaling = 0.99;

const double kPrimalRegularization = 1e-10;
const double kDualRegularization = 0;

#endif