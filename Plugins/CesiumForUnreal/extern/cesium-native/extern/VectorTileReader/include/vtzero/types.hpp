#ifndef VTZERO_TYPES_HPP
#define VTZERO_TYPES_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

#include <protozero/pbf_reader.hpp>

#include <array>
#include <cassert>

// @cond internal
// Wrappers for assert() used for testing
#ifndef vtzero_assert
# define vtzero_assert(x) assert(x)
#endif
#ifndef vtzero_assert_in_noexcept_function
# define vtzero_assert_in_noexcept_function(x) assert(x)
#endif
// @endcond

/**
 * @file types.hpp
 *
 * @brief Contains the declaration of low-level types.
 */

/**
 * @brief All parts of the vtzero header-only library are in this namespace.
 */
namespace vtzero {

    /**
     * Using data_view class from protozero. See the protozero documentation
     * on how to change this to use a different implementation.
     * https://github.com/mapbox/protozero/blob/master/doc/advanced.md#protozero_use_view
     */
    using data_view = protozero::data_view;

    // based on https://github.com/mapbox/vector-tile-spec/blob/master/2.1/vector_tile.proto

    /// The geometry type as specified in the vector tile spec
    enum class GeomType : int32_t {
        UNKNOWN    = 0,
        POINT      = 1,
        LINESTRING = 2,
        POLYGON    = 3,
        SPLINE     = 4,
        max        = 4
    };

    /**
     * Return the name of a GeomType (for debug output etc.)
     */
    inline const char* geom_type_name(GeomType type) noexcept {
        static const std::array<const char*, 5> names = {
            {"unknown", "point", "linestring", "polygon", "spline"}
        };
        return names[static_cast<std::size_t>(type)]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    /**
     * Type of a polygon ring. This can either be "outer", "inner", or
     * "invalid". Invalid is used when the area of the ring is 0.
     */
    enum class ring_type {
        outer   = 0,
        inner   = 1,
        invalid = 2
    }; // enum class ring_type

    /// The property value type as specified in the vector tile spec
    enum class property_value_type : protozero::pbf_tag_type {
        string_value = 1,
        float_value  = 2,
        double_value = 3,
        int_value    = 4,
        uint_value   = 5,
        sint_value   = 6,
        bool_value   = 7
    };

    /**
     * Return the name of a property value type (for debug output etc.)
     */
    inline const char* property_value_type_name(property_value_type type) noexcept {
        static const std::array<const char*, 8> names = {
            {"", "string", "float", "double", "int", "uint", "sint", "bool"}
        };
        return names[static_cast<std::size_t>(type)]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    namespace detail {

        enum class pbf_tile : protozero::pbf_tag_type {
            layers = 3
        };

        enum class pbf_scaling : protozero::pbf_tag_type {
            offset     = 1,
            multiplier = 2,
            base       = 3
        };

        enum class pbf_layer : protozero::pbf_tag_type {
            name               =  1,
            features           =  2,
            keys               =  3,
            values             =  4,
            extent             =  5,
            string_values      =  6,
            float_values       =  7,
            double_values      =  8,
            int_values         =  9,
            elevation_scaling  = 10,
            attribute_scalings = 11,
            tile_x             = 12,
            tile_y             = 13,
            tile_zoom          = 14,
            version            = 15
        };

        enum class pbf_feature : protozero::pbf_tag_type {
            id                   =  1,
            tags                 =  2,
            type                 =  3,
            geometry             =  4,
            attributes           =  5,
            geometric_attributes =  6,
            elevations           =  7,
            spline_knots         =  8,
            spline_degree        =  9,
            string_id            = 10
        };

        using pbf_value = property_value_type;

    } // namespace detail

    /// The "null" value type.
    struct null_type {
    };

    /// property value type holding a reference to a string
    struct string_value_type {

        /// the underlying storage type
        using type = data_view;

        /// @cond internal
        constexpr static const property_value_type pvtype = property_value_type::string_value;
        constexpr static const protozero::pbf_wire_type wire_type = protozero::pbf_wire_type::length_delimited;
        /// @endcond

        /// value
        data_view value{};

        /// Construct empty string_value_type
        constexpr string_value_type() noexcept = default;

        /// Construct string_value_type
        explicit constexpr string_value_type(data_view v) :
            value(v) {
        }

    }; // struct string_value_type

    /// property value type holding a float
    struct float_value_type {

        /// the underlying storage type
        using type = float;

        /// @cond internal
        constexpr static const property_value_type pvtype = property_value_type::float_value;
        constexpr static const protozero::pbf_wire_type wire_type = protozero::pbf_wire_type::fixed32;
        /// @endcond

        /// value
        float value = 0.0F;

        /// Construct float_value_type with value 0.0
        constexpr float_value_type() noexcept = default;

        /// Construct float_value_type
        explicit constexpr float_value_type(float v) :
            value(v) {
        }

    }; // struct float_value_type

    /// property value type holding a double
    struct double_value_type {

        /// the underlying storage type
        using type = double;

        /// @cond internal
        constexpr static const property_value_type pvtype = property_value_type::double_value;
        constexpr static const protozero::pbf_wire_type wire_type = protozero::pbf_wire_type::fixed64;
        /// @endcond

        /// value
        double value = 0.0;

        /// Construct double_value_type with value 0.0
        constexpr double_value_type() noexcept = default;

        /// Construct double_value_type
        explicit constexpr double_value_type(double v) :
            value(v) {
        }

    }; // struct double_value_type

    /// property value type holding an int
    struct int_value_type {

        /// the underlying storage type
        using type = int64_t;

        /// @cond internal
        constexpr static const property_value_type pvtype = property_value_type::int_value;
        constexpr static const protozero::pbf_wire_type wire_type = protozero::pbf_wire_type::varint;
        /// @endcond

        /// value
        int64_t value = 0;

        /// Construct int_value_type with value 0
        constexpr int_value_type() noexcept = default;

        /// Construct int_value_type
        explicit constexpr int_value_type(int64_t v) :
            value(v) {
        }

    }; // struct int_value_type

    /// property value type holding a uint
    struct uint_value_type {

        /// the underlying storage type
        using type = uint64_t;

        /// @cond internal
        constexpr static const property_value_type pvtype = property_value_type::uint_value;
        constexpr static const protozero::pbf_wire_type wire_type = protozero::pbf_wire_type::varint;
        /// @endcond

        /// value
        uint64_t value = 0;

        /// Construct uint_value_type with value 0
        constexpr uint_value_type() noexcept = default;

        /// Construct uint_value_type
        explicit constexpr uint_value_type(uint64_t v) :
            value(v) {
        }

    }; // struct uint_value_type

    /// property value type holding an sint
    struct sint_value_type {

        /// the underlying storage type
        using type = int64_t;

        /// @cond internal
        constexpr static const property_value_type pvtype = property_value_type::sint_value;
        constexpr static const protozero::pbf_wire_type wire_type = protozero::pbf_wire_type::varint;
        /// @endcond

        /// value
        int64_t value = 0;

        /// Construct sint_value_type with value 0
        constexpr sint_value_type() noexcept = default;

        /// Construct sint_value_type
        explicit constexpr sint_value_type(int64_t v) :
            value(v) {
        }

    }; // struct sint_value_type

    /// property value type holding a bool
    struct bool_value_type {

        /// the underlying storage type
        using type = bool;

        /// @cond internal
        constexpr static const property_value_type pvtype = property_value_type::bool_value;
        constexpr static const protozero::pbf_wire_type wire_type = protozero::pbf_wire_type::varint;
        /// @endcond

        /// value
        bool value = false;

        /// Construct bool_value_type with false value
        constexpr bool_value_type() noexcept = default;

        /// Construct bool_value_type
        explicit constexpr bool_value_type(bool v) :
            value(v) {
        }

    }; // struct bool_value_type

    /**
     * This class wraps the uint32_t used for looking up keys/values in the
     * key/values tables.
     */
    class index_value {

        enum : uint32_t {
            invalid_value = std::numeric_limits<uint32_t>::max()
        };

        uint32_t m_value = invalid_value;

    public:

        /// Default construct to an invalid value.
        constexpr index_value() noexcept = default;

        /// Construct with the given value.
        constexpr index_value(uint32_t value) noexcept : // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
            m_value(value) {
        }

        /**
         * Is this index value valid? Index values are valid if they have
         * been initialized with something other than the default constructor.
         */
        constexpr bool valid() const noexcept {
            return m_value != invalid_value;
        }

        /**
         * Get the value.
         *
         * @pre Must be valid.
         */
        uint32_t value() const noexcept {
            vtzero_assert_in_noexcept_function(valid());
            return m_value;
        }

    }; // class index_value

    /// Index values are equal if their values are.
    inline bool operator==(const index_value lhs, const index_value rhs) noexcept {
        return lhs.value() == rhs.value();
    }

    /// Index values are not equal if their values are not equal.
    inline bool operator!=(const index_value lhs, const index_value rhs) noexcept {
        return lhs.value() != rhs.value();
    }

    /**
     * This class holds two index_values, one for a key and one for a value.
     */
    class index_value_pair {

        index_value m_key{};
        index_value m_value{};

    public:

        /// Default construct to an invalid value.
        constexpr index_value_pair() noexcept = default;

        /// Construct with the given values.
        constexpr index_value_pair(index_value key, index_value value) noexcept :
            m_key(key),
            m_value(value) {
        }

        /**
         * Is this index value pair valid? Index values pairs are valid if
         * both the key and the value index value are valid.
         */
        constexpr bool valid() const noexcept {
            return m_key.valid() && m_value.valid();
        }

        /**
         * Is this index value pair valid? Index values pairs are valid if
         * both the key and the value index value are valid.
         */
        constexpr explicit operator bool() const noexcept {
            return valid();
        }

        /// Get the key index value.
        constexpr index_value key() const noexcept {
            return m_key;
        }

        /// Get the value index value.
        constexpr index_value value() const noexcept {
            return m_value;
        }

    }; // class index_value_pair

} // namespace vtzero

#endif // VTZERO_TYPES_HPP
