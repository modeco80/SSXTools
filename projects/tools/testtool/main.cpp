#include <fstream>
#include <ssxtools/EacRiffChunker.hpp>

int main() {
	// std::ifstream ifs{"/data/sda/lily/games/ssx/ssx3/data/worlds/data/worlds/bam.ssb", std::ifstream::binary};
	std::ifstream ifs { "/data/sda/lily/games/ssx/ssx3/data/movies/eabig.mpc", std::ifstream::binary };

	ssxtools::core::EacRiffChunker chunker { ifs };
	while(true) {
		if(!chunker.ReadChunk([&](u32 fourCC, const std::vector<u8>& data) {
			   auto fcc_bytes = std::bit_cast<char*>(&fourCC);
			   std::cout << "Got chunk: Type " << fcc_bytes[0] << fcc_bytes[1] << fcc_bytes[2] << fcc_bytes[3] << " data size " << data.size() << " bytes\n";
		   }))
			break;
	}

	return 0;
}
