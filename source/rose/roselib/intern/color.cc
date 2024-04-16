#include "LIB_color.hh"

#include <ostream>

namespace rose {

std::ostream &operator<<(std::ostream &stream, const eAlpha &space) {
	switch (space) {
		case eAlpha::Straight: {
			stream << "Straight";
			break;
		}
		case eAlpha::Premultiplied: {
			stream << "Premultiplied";
			break;
		}
	}
	return stream;
}

std::ostream &operator<<(std::ostream &stream, const eSpace &space) {
	switch (space) {
		case eSpace::Theme: {
			stream << "Theme";
			break;
		}
		case eSpace::SceneLinear: {
			stream << "SceneLinear";
			break;
		}
		case eSpace::SceneLinearByteEncoded: {
			stream << "SceneLinearByteEncoded";
			break;
		}
	}
	return stream;
}

}  // namespace rose
