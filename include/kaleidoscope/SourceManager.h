//
// Created by Sergej Jaskiewicz on 2019-05-30.
//

#ifndef KALEIDOSCOPE_SOURCEMANAGER_H
#define KALEIDOSCOPE_SOURCEMANAGER_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/ADT/Optional.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/VirtualFileSystem.h"

namespace kaleidoscope {

class SourceManager {
  llvm::SourceMgr LLVMSourceMgr;
  llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> FileSystem;

  /// Associates buffer identifiers to buffer IDs.
  llvm::DenseMap<llvm::StringRef, unsigned> BufIdentIDMap;

public:
  explicit SourceManager(llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> FS =
                             llvm::vfs::getRealFileSystem())
      : FileSystem(std::move(FS)) {}

  llvm::SourceMgr &getLLVMSourceMgr() { return LLVMSourceMgr; }

  const llvm::SourceMgr &getLLVMSourceMgr() const { return LLVMSourceMgr; }

  /// Adds a memory buffer to the SourceManager, taking ownership of it.
  unsigned addNewSourceBuffer(std::unique_ptr<llvm::MemoryBuffer> Buffer);

  /// Creates and adds a memory buffer to the \c SourceManager, taking
  /// ownership of the newly created copy.
  ///
  /// \p InputData and \p BufIdentifier are copied, so that this memory can go
  /// away as soon as this function returns.
  unsigned addMemBufferCopy(llvm::StringRef InputData,
                            llvm::StringRef BufIdentifier = "");

  /// Returns a SMRange covering the entire specified buffer.
  ///
  /// Note that the start location might not point at the first token: it
  /// might point at whitespace or a comment.
  llvm::SMRange getRangeForBuffer(unsigned BufferID) const;

  llvm::StringRef
  extractText(llvm::SMRange Range,
              llvm::Optional<unsigned> BufferID = llvm::None) const;

  unsigned findBufferContainingLoc(llvm::SMLoc Loc) const;

  unsigned int getLocOffsetInBuffer(llvm::SMLoc Loc,
                                    unsigned int BufferID) const;

  /// Returns the SMLoc for the beginning of the specified buffer
  /// (at offset zero).
  ///
  /// Note that the resulting location might not point at the first token: it
  /// might point at whitespace or a comment.
  llvm::SMLoc getLocForBufferStart(unsigned BufferID) const {
    return getRangeForBuffer(BufferID).Start;
  }

  /// Returns the SourceLoc for the byte offset in the specified buffer.
  llvm::SMLoc getLocForOffset(unsigned BufferID, unsigned Offset) const {
    return llvm::SMLoc::getFromPointer(
        getLocForBufferStart(BufferID).getPointer() + Offset);
  }
};

} // namespace kaleidoscope

#endif /* KALEIDOSCOPE_SOURCEMANAGER_H */
