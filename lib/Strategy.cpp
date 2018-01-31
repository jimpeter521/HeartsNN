#include "lib/Strategy.h"
#include "lib/Annotator.h"

Strategy::~Strategy() {}

Strategy::Strategy(const AnnotatorPtr& annotator)
: mAnnotator(annotator)
{}
