#include <vector>
#include <sstream>
#include <eagle/ShpsWriter.h>
#include <eagle/Core.h>

// TODO: Switch to a real libpng
#include "stb_image_write.h"

using namespace eagle::core;

namespace eagle::core {

		/**
		 * Multiply an stored byte.
		 *
		 * \param[in] val val.
		 */
		inline byte MultiplyValue(byte val) {
			// Multiply the stored value by 2
			// or round it directly up to 255 if it's 128.
			// We do this instead of blindly multiplying the value
			// because it could overflow and break images.
			if(val < 128)
				return val * 2;
			else if(val == 128) // is another branch needed?
				return 255;

			// I don't know how you'd ever get beyond the following point
			// but OK
			return 255;
		}

		// TODO: we should only detour to this if the user doesn't give an option
		// whether in the UI (i don't know where I'd put this) or in the cli (say --disable-ssxhack or --enable-ssxhack)
		/**
		 * Function called **ONCE** on the ShpsImage to determine if it possibly
		 * went through the fabled
		 * `FIXSSH.BAT` (the Winter Jampack 2001 build of SSX Tricky accidentally shipped with this)
		 * aka `gx -pixela/2,r/2,g/2,b/2 -csm2 %1`.
		 * If it did, the hack should be enabled.
		 */
		bool ShouldEnableSSXHack(shps::Image& image) {
			// test
			auto test = [](byte c) {
				return c != 255 && c != 0 /* Seems to fix some other textures */ && std::max(c, (byte)128) == 128;
			};

			shps::Bgra8888 MaxColor{};

			if(image.palette.empty()) {
				// Detour to a much slower impl
				// that gets the max pixel in the entire image.
				// This will be far slower than if we had a palette,
				// but it's the only way we can determine it on 32bpp images.

				MaxColor = MaxSpanElement(mco::MakeSpan((shps::Bgra8888*)image.data.data(), image.width * image.height), [](const shps::Bgra8888& l, const shps::Bgra8888& r) {
					return std::max(l.r, r.r) && std::max(l.g, r.g) && std::max(l.b, r.b) && std::max(l.a, r.a);
				});
			} else {
				// Use a faster method that just gets the max palette.

				MaxColor = MaxSpanElement(mco::MakeSpan(image.palette.data(), image.palette.size()), [](const shps::Bgra8888& l, const shps::Bgra8888& r) {
					return std::max(l.r, r.r) && std::max(l.g, r.g) && std::max(l.b, r.b) && std::max(l.a, r.a);
				});

			}

			// seems the best way to detect this hack is to test just the alpha
			return test(MaxColor.a);
		}

		// Constant amount of channels in the output PNG.
		constexpr int32 CHANNEL_COUNT = 4;

		bool ShpsWriter::BuildImageBuffer(std::vector<byte>& imageBuffer, core::shps::Image& image) {
			if(image.data.empty()) {
				//logger.error("Image ", image.index, " is empty or unknown format!");
				return false;
			}

			bool ssxHack = ShouldEnableSSXHack(image);

			//if(ssxHack)
			//	logger.info("Enabling FIXSSH.BAT hack cause alpha was <= 128");

			//logger.info("Writing image ", image.index, "...");

			//logger.info("Information on image ", image.index, ':');
			//logger.info("Width x Height: ", image.width, 'x', image.height);

			imageBuffer.resize((image.width * image.height * CHANNEL_COUNT));

			switch(image.format) {
				case shps::ShpsImageType::Lut128: {
					//logger.info("Image ", image.index, " is an 4bpp image.");
					byte* normalizedDataPtr = imageBuffer.data();

					// Current pixel.
					byte* texPixelPtr = image.data.data();

					// Write each pixel to the image buffer that we save.
					// We do this by looking in the LUT for each pixel and setting the colors there.
					for(int i = 0; i < image.width * image.height; ++i) {
						int index = 0;

						for(int b = 0; b < 2; b++) {
							index = ((*texPixelPtr & (0x0F, (b * 4))) >> (b * 4));
						}

						*(normalizedDataPtr++) = image.palette[index].b;
						*(normalizedDataPtr++) = image.palette[index].g;
						*(normalizedDataPtr++) = image.palette[index].r;
						*(normalizedDataPtr++) = MultiplyValue(image.palette[index].a);

						texPixelPtr++;
					}
				} break;

				case shps::ShpsImageType::Lut256: {
					//logger.info("Image ", image.index, " is an 8bpp image.");
					byte* normalizedDataPtr = imageBuffer.data();

					// Current pixel.
					byte* texPixelPtr = image.data.data();

					// Write each pixel to the image buffer that we save.
					// We do this by looking in the LUT for each pixel and setting the colors there.
					for(int i = 0; i < image.width * image.height; ++i) {
						if(ssxHack) {
							*(normalizedDataPtr++) = MultiplyValue(image.palette[*texPixelPtr].b);
							*(normalizedDataPtr++) = MultiplyValue(image.palette[*texPixelPtr].g);
							*(normalizedDataPtr++) = MultiplyValue(image.palette[*texPixelPtr].r);
							*(normalizedDataPtr++) = MultiplyValue(image.palette[*texPixelPtr].a);
						} else {
							*(normalizedDataPtr++) = image.palette[*texPixelPtr].b;
							*(normalizedDataPtr++) = image.palette[*texPixelPtr].g;
							*(normalizedDataPtr++) = image.palette[*texPixelPtr].r;
							*(normalizedDataPtr++) = MultiplyValue(image.palette[*texPixelPtr].a);
						}

						texPixelPtr++;
					}
				} break;

				case shps::ShpsImageType::NonLut32Bpp: {
					//logger.info("Image ", image.index, " is an 32bpp image.");

					byte* normalizedDataPtr = imageBuffer.data();

					// We cast the image data (which is just individual bytes) to Bgra8888* because
					// non-LUT images directly use Bgra8888.
					//
					// Also saves a bit of typing, as we won't have to manually
					// advance 4 bytes and cast to Bgra8888* each and every time.
					// We can just increment the pointer!
					auto* texPixelPtr = (shps::Bgra8888*)image.data.data();

					// Write each pixel to the image buffer that we save.
					for(int i = 0; i < image.width * image.height; ++i) {
						if(ssxHack) {
							*(normalizedDataPtr++) = MultiplyValue((*texPixelPtr).b);
							*(normalizedDataPtr++) = MultiplyValue((*texPixelPtr).g);
							*(normalizedDataPtr++) = MultiplyValue((*texPixelPtr).r);
							*(normalizedDataPtr++) = MultiplyValue((*texPixelPtr).a);
						} else {
							*(normalizedDataPtr++) = (*texPixelPtr).b;
							*(normalizedDataPtr++) = (*texPixelPtr).g;
							*(normalizedDataPtr++) = (*texPixelPtr).r;
							*(normalizedDataPtr++) = MultiplyValue((*texPixelPtr).a);
						}

						texPixelPtr++;
					}
				} break;

				default:
					break;
			}
			return true;
		}

		bool ShpsWriter::WritePNG(core::shps::Image& image, const std::filesystem::path& input_path, const std::filesystem::path& output_path) {
			// avoid weird images entirely,
			// helps avoid crashing on certain things
			if(image.data.empty()) {
				logger.error("Image ", image.index, " is empty or unknown format!");
				return false;
			}

			auto path = output_path / input_path.filename();
			std::filesystem::create_directories(path);
			std::string outFilename = (path / std::filesystem::path(std::to_string(image.index)).replace_extension(".PNG")).string();

			std::vector<byte> imageData;

			// build image buffer
			if(!BuildImageBuffer(imageData, image)) {
				logger.error("Could not build image buffer...");
			}


			// Finally, write the PNG after we've made the data buffers.
			stbi_write_png(outFilename.c_str(), image.width, image.height, CHANNEL_COUNT, imageData.data(), (image.width * CHANNEL_COUNT));
			logger.info("Image ", image.index, " written to \"", outFilename, "\".");

			// Clear the PNG data buffer after we're done.
			imageData.clear();

			return true;
		}

	} // namespace eagle