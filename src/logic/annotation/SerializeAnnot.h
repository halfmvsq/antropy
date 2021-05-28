#ifndef SERIALIZE_ANNOT_H
#define SERIALIZE_ANNOT_H

#include "logic/annotation/Annotation.h"

#include <nlohmann/json.hpp>

void to_json( nlohmann::json&, const AnnotPolygon<float, 2>& );
void from_json( const nlohmann::json&, AnnotPolygon<float, 2>& );

void to_json( nlohmann::json&, const Annotation& );
void from_json( const nlohmann::json&, Annotation& );

#endif // SERIALIZE_ANNOT_H
