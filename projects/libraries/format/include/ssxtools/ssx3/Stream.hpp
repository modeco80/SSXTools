#pragma once

#include <ssxtools/ssx3/RID.hpp>
#include <ssxtools/Types.hpp>
#include <ssxtools/uint24.hpp>

namespace ssxtools::format::ssx3 {

	struct InterleaveHeader {
		enum class InterleaveType : u8 {
			/// I have no idea if these are correct
			Unk = 0,
			Patch = 1,
			Model = 2,
			Instance = 3,

			/// These ones are

			StreamTexture = 9,
			LightmapTexture = 10,

			AudioBank = 20, /// An audio bank

			BeforeEnd = 0x12, // 0x48 bytes of padding, unknown purpose
			End = 0x16		  // no data?
		};

		/// The resource identifier of this interleaved resource
		/// (where it goes basically?)
		RID rid;

		/// The type of this interleaved resource
		InterleaveType type;

		/// The data size of this interleaved resource.
		u24<> dataSize;

		/// Get the data of this resource.
		constexpr u8* Data() { return std::bit_cast<u8*>(this + 1); }

		/// Get to the next interleave entry
		constexpr InterleaveHeader* NextEntry() const {
			if(dataSize == 0)
				return nullptr;

			return std::bit_cast<InterleaveHeader*>(std::bit_cast<u8*>(this) + dataSize);
		}

		/// Find a resource with the given RID.
		constexpr InterleaveHeader* FindRID(const RID rid) {
			auto p = this;
			while(p != nullptr) {
				if(p->rid == rid)
					return p;

				// move on to the next entry
				p = p->NextEntry();
			}

			return nullptr;
		}
	};

	// For documentation's sake:
	//
	// The [x|g|s]sb file is essentially a bunch of aligned EAC-RIFF chunks since that's the only thing EAC's
	// STREAM_ library can actually process (and the streaming uses that library)
	//
	// These chunks have RefPack compressed data:
	// 	'CBXS' ident for regular chunks
	// 	'CEND' ident for end chunks (this signifies the end of a given interleave stream)
	//
	// The RefPack data is decompressed and weaved into one file

} // namespace ssxtools::format::ssx3
