//
// Created by Sergej Jaskiewicz on 2019-05-30.
//

#include "kaleidoscope/SourceManager.h"

using namespace kaleidoscope;
using namespace llvm;

unsigned
SourceManager::addNewSourceBuffer(std::unique_ptr<MemoryBuffer> Buffer) {
  assert(Buffer);
  StringRef BufIdentifier = Buffer->getBufferIdentifier();
  unsigned ID = LLVMSourceMgr.AddNewSourceBuffer(std::move(Buffer), SMLoc());
  BufIdentIDMap.try_emplace(BufIdentifier, ID);
  return ID;
}

unsigned SourceManager::addMemBufferCopy(StringRef InputData,
                                         StringRef BufIdentifier) {
  return addNewSourceBuffer(
      MemoryBuffer::getMemBufferCopy(InputData, BufIdentifier));
}

SMRange SourceManager::getRangeForBuffer(unsigned BufferID) const {
  const MemoryBuffer *buffer = LLVMSourceMgr.getMemoryBuffer(BufferID);
  auto start = SMLoc::getFromPointer(buffer->getBufferStart());
  auto end = SMLoc::getFromPointer(buffer->getBufferEnd());
  return SMRange(start, end);
}

unsigned SourceManager::findBufferContainingLoc(SMLoc Loc) const {
  assert(Loc.isValid());

  return LLVMSourceMgr.FindBufferContainingLoc(Loc);
}

unsigned SourceManager::getLocOffsetInBuffer(llvm::SMLoc Loc,
                                             unsigned BufferID) const {
  assert(Loc.isValid() && "location should be valid");

  const MemoryBuffer *Buffer = LLVMSourceMgr.getMemoryBuffer(BufferID);

  assert(Loc.getPointer() >= Buffer->getBuffer().begin() &&
         Loc.getPointer() <= Buffer->getBuffer().end() &&
         "Location is not from the specified buffer");

  return Loc.getPointer() - Buffer->getBuffer().begin();
}

StringRef SourceManager::extractText(SMRange Range,
                                     Optional<unsigned int> BufferID) const {

  assert(Range.isValid() && "range should be valid");

  if (!BufferID) {
    BufferID = findBufferContainingLoc(Range.Start);
  }

  StringRef Buffer = LLVMSourceMgr.getMemoryBuffer(*BufferID)->getBuffer();

  unsigned ByteLength = Range.End.getPointer() - Range.Start.getPointer();

  return Buffer.substr(getLocOffsetInBuffer(Range.Start, *BufferID),
                       ByteLength);
}
