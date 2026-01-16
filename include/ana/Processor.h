#pragma once

namespace ROOT::RDF {
class RNode;
}

namespace strangeness {

struct Entry;

class Processor {
public:
  virtual ~Processor() = default;

  virtual ROOT::RDF::RNode run(ROOT::RDF::RNode node, const Entry&) const;
};

const Processor& processor();

}  // namespace strangeness
