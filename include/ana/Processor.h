#pragma once

#include <ROOT/RDF/RNode.hxx>

namespace strangeness {

struct Entry;

class Processor {
public:
  virtual ~Processor() = default;

  virtual ROOT::RDF::RNode run(ROOT::RDF::RNode node, const Entry&) const;
};

const Processor& processor();

}  // namespace strangeness
