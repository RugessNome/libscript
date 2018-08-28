// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_CONVERSIONS_H
#define LIBSCRIPT_CONVERSIONS_H

#include "script/function.h"
#include "script/types.h"

namespace script
{

namespace program
{
class Expression;
} // namespace program

enum NumericPromotion {
  NoNumericPromotion = 0,
  IntegralPromotion = 2,
  FloatingPointPromotion = 4,
};

enum NumericConversion {
  NoNumericConversion = 0,
  IntegralConversion = 8,
  FloatingPointConversion = 16,
  BooleanConversion = 32,
};

enum QualificationAdjustment {
  NoQualificationAdjustment = 0,
  ConstQualification = 1,
};

struct DerivedToBaseConversion {
  int depth;
  
  static const int Shift = 16;
  operator int() const { return depth << Shift; }
};

class LIBSCRIPT_API StandardConversion
{
public:
  StandardConversion();
  StandardConversion(const StandardConversion &) = default;
  ~StandardConversion() = default;

  enum Flag {
    ReferenceInitializationFlag = 1 << 24,
    CopyInitializationFlag = 2 << 24,
    CopyRefInitializationFlag = 3 << 24,
    NotConvertibleFlag = 4 << 24,
  };

  enum class Rank {
    ExactMatch = 1,
    Promotion = 2,
    Conversion = 3,
    NotConvertible = 5,
  };

  StandardConversion(NumericConversion conv, QualificationAdjustment qualadjust = QualificationAdjustment::NoQualificationAdjustment);
  StandardConversion(NumericPromotion conv, QualificationAdjustment qualadjust = QualificationAdjustment::NoQualificationAdjustment);
  StandardConversion(DerivedToBaseConversion dbconv, QualificationAdjustment qualadjust = QualificationAdjustment::NoQualificationAdjustment);
  StandardConversion(QualificationAdjustment qualadjust);

  bool isNone() const;
  static StandardConversion None();

  bool isNarrowing() const;
  Rank rank() const;

  bool isCopyInitialization() const;
  bool isReferenceInitialization() const;

  bool isNumericPromotion() const;
  NumericPromotion numericPromotion() const;

  bool isNumericConversion() const;
  NumericConversion numericConversion() const;

  bool hasQualificationConversion() const;
  QualificationAdjustment qualificationConversion();

  bool isDerivedToBaseConversion() const;
  int derivedToBaseConversionDepth() const;

  StandardConversion with(QualificationAdjustment adjust) const;

  static StandardConversion NotConvertible();
  static StandardConversion ReferenceInitialization();
  static StandardConversion ReferenceInitialization(DerivedToBaseConversion dbconv, QualificationAdjustment qualadjust = QualificationAdjustment::NoQualificationAdjustment);
  static StandardConversion ReferenceInitialization(QualificationAdjustment qualadjust);
  static StandardConversion CopyRefInitialization(StandardConversion conv);

  static StandardConversion compute(const Type & src, const Type & dest, Engine *e);

  bool operator==(const StandardConversion & other) const;
  inline bool operator!=(const StandardConversion & other) const { return !((*this) == other); }
  bool operator<(const StandardConversion & other) const;

private:
  int d;
};

class LIBSCRIPT_API ListInitializationSequence;

/// TODO : implement a diagnostic() method that returns error messges
class LIBSCRIPT_API ConversionSequence
{
public:
  ConversionSequence() = default;
  ConversionSequence(const ConversionSequence &) = default;
  ~ConversionSequence() = default;
  ConversionSequence(const StandardConversion & c1, const Function & userdefinedConversion = Function{}, const StandardConversion & c2 = StandardConversion::None());
  ConversionSequence(const std::shared_ptr<ListInitializationSequence> & listinit);

  enum class Rank {
    ExactMatch = 1,
    Promotion = 2,
    Conversion = 3,
    UserDefinedConversion = 4,
    NotConvertible = 5,
  };

  Rank rank() const;

  static Rank globalRank(const std::vector<ConversionSequence> & convs);

  bool isInvalid() const;
  bool isNarrowing() const;
  bool isUserDefinedConversion() const;
  bool isListInitialization() const;

  StandardConversion conv1;
  std::shared_ptr<ListInitializationSequence> listInitialization;
  Function function;
  StandardConversion conv3;

  static ConversionSequence NotConvertible();

  static ConversionSequence compute(const Type & src, const Type & dest, Engine *engine);
  static ConversionSequence compute(const std::shared_ptr<program::Expression> & expr, const Type & dest, Engine *engine);

  static int comp(const ConversionSequence & a, const ConversionSequence & b);

  bool operator==(const ConversionSequence & other) const;
  inline bool operator!=(const ConversionSequence & other) const { return !((*this) == other); }
};


class LIBSCRIPT_API ListInitializationSequence
{
public:

  enum Kind {
    None = 0,
    DefaultInitialization = 1,
    InitializerListCreation = 2,
    InitializerListInitialization = 3,
    ConstructorListInitialization = 4,
    AggregateListInitialization = 5, // currently not used
  };

  ListInitializationSequence(Type t)
    : mKind(DefaultInitialization)
    , mType(t) {}

  ListInitializationSequence(Kind k, Type t);
  ListInitializationSequence(Kind k, Function ctor);


  inline Kind kind() const { return mKind; }
  inline Type destType() const { return mType; }
  inline Function constructor() const { return mConstructor; }

  std::vector<ConversionSequence> & conversions();
  const std::vector<ConversionSequence> & conversions() const;

  static int comp(const ListInitializationSequence & a, const ListInitializationSequence & b);

private:
  Kind mKind;
  Type mType;
  Function mConstructor;
  std::vector<ConversionSequence> mConversions;
};



enum class ConversionRank {
  ExactMatch = 1,
  Promotion = 2,
  Conversion = 3,
  UserDefinedConversion = 4,
  NotConvertible = 5,
};

class LIBSCRIPT_API StandardConversion2
{
public:
  StandardConversion2();
  StandardConversion2(const Type & src, const Type & dest);
  StandardConversion2(const StandardConversion2 &) = default;
  ~StandardConversion2() = default;

  StandardConversion2(QualificationAdjustment qualadjust);

  bool isNone() const;
  static StandardConversion2 None();

  bool isNarrowing() const;
  ConversionRank rank() const;

  bool isReferenceConversion() const;

  bool isNumericPromotion() const;
  NumericPromotion numericPromotion() const;

  bool isNumericConversion() const;
  NumericConversion numericConversion() const;

  bool hasQualificationAdjustment() const;

  bool isDerivedToBaseConversion() const;
  int derivedToBaseConversionDepth() const;

  Type srcType() const;
  Type destType() const;

  StandardConversion2 with(QualificationAdjustment adjust) const;

  static StandardConversion2 Copy();
  static StandardConversion2 EnumToInt();
  static StandardConversion2 DerivedToBaseConversion(int depth, bool is_ref_conv, QualificationAdjustment adjust = QualificationAdjustment::NoQualificationAdjustment);
  static StandardConversion2 NotConvertible();

  static StandardConversion2 compute(const Type & src, const Type & dest, Engine *e);

  bool operator==(const StandardConversion2 & other) const;
  inline bool operator!=(const StandardConversion2 & other) const { return !((*this) == other); }
  bool operator<(const StandardConversion2 & other) const;

private:
  StandardConversion2(int val) : d(val) { }

private:
  int d;
};


class LIBSCRIPT_API Conversion
{
public:
  Conversion() = default;
  Conversion(const Conversion &) = default;
  ~Conversion() = default;
  Conversion(const StandardConversion2 & c1, const Function & userdefinedConversion = Function{}, const StandardConversion2 & c2 = StandardConversion2::None());

  ConversionRank rank() const;

  static ConversionRank globalRank(const std::vector<Conversion> & convs);

  bool isInvalid() const;
  bool isNarrowing() const;
  bool isUserDefinedConversion() const;

  const StandardConversion2 & firstStandardConversion() const { return conv1; }
  const Function & userDefinedConversion() const { return function; }
  const StandardConversion2 & secondStandardConversion() const { return conv3; }

  Type srcType() const;
  Type destType() const;

  static Conversion NotConvertible();

  enum ConversionPolicy {
    NoExplicitConversions = 0,
    AllowExplicitConversions,
  };

  static Conversion compute(const Type & src, const Type & dest, Engine *engine, ConversionPolicy policy = NoExplicitConversions);
  static Conversion compute(const std::shared_ptr<program::Expression> & expr, const Type & dest, Engine *engine);

  static int comp(const Conversion & a, const Conversion & b);

  bool operator==(const Conversion & other) const;
  inline bool operator!=(const Conversion & other) const { return !((*this) == other); }

private:
  StandardConversion2 conv1;
  Function function;
  StandardConversion2 conv3;
};

} // namespace script

#endif // LIBSCRIPT_CONVERSIONS_H
