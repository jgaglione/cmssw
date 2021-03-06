#ifndef FWCore_Framework_MessageForParent_h
#define FWCore_Framework_MessageForParent_h
// -*- C++ -*-
//
// Package:     Framework
// Class  :     MessageForParent
//
/**\class MessageForSource MessageForParent.h FWCore/Framework/interface/MessageForParent.h

 Description: Information passed from parent to source when doing multicore processing

 Usage:
    This class is an internal detail of how the child process communicates with the parent.
 It is sent across a Unix socket to the parent to indicate that the child needs work.

*/
//
// Original Author:  Brian Bockelman
//         Created:  Sat Oct 22 10:34:00 CEST 2011
//

// system include files
#include <cstddef>

// user include files

// forward declarations

namespace edm {
  namespace multicore {
    class MessageForParent {
    public:
      MessageForParent() : m_dummy(0) {}

      //virtual ~MessageForSource();

      // ---------- const member functions ---------------------

      // ---------- static member functions --------------------
      static size_t sizeForBuffer() { return sizeof(MessageForParent); }

    public:
      // ---------- member functions ---------------------------

      //MessageForSource(const MessageForSource&); // allow default

      //const MessageForSource& operator=(const MessageForSource&); // allow default

      // ---------- member data --------------------------------
      int m_dummy;
    };

  }  // namespace multicore
}  // namespace edm

#endif
