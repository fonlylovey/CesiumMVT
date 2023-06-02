#ifndef VTZERO_SCALING_HPP
#define VTZERO_SCALING_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file scaling.hpp
 *
 * @brief Contains the scaling class.
 */

#include "exception.hpp"
#include "types.hpp"

#include <protozero/basic_pbf_builder.hpp>
#include <protozero/pbf_message.hpp>

#include <cstdint>
#include <vector>

namespace vtzero {

    /**
     * @brief Scaling parameters
     *
     * A scaling according to spec 4.4.2.5.
     */
    class scaling {

        int64_t m_offset = 0;

        double m_multiplier = 1.0;

        double m_base = 0.0;

    public:

        /// Construct scaling with default values.
        constexpr scaling() noexcept = default;

        /// Construct scaling with specified values.
        constexpr scaling(int64_t offset, double multiplier, double base) noexcept :
            m_offset(offset),
            m_multiplier(multiplier),
            m_base(base) {
        }

        /// Construct scaling from pbf message.
        explicit scaling(data_view message) {
            protozero::pbf_message<detail::pbf_scaling> reader{message};
            while (reader.next()) {
                switch (reader.tag()) {
                    case detail::pbf_scaling::offset:
                        if (reader.wire_type() != protozero::pbf_wire_type::varint) {
                            throw format_exception{"Scaling offset has wrong protobuf type"};
                        }
                        m_offset = reader.get_sint64();
                        break;
                    case detail::pbf_scaling::multiplier:
                        if (reader.wire_type() != protozero::pbf_wire_type::fixed64) {
                            throw format_exception{"Scaling multiplier has wrong protobuf type"};
                        }
                        m_multiplier = reader.get_double();
                        break;
                    case detail::pbf_scaling::base:
                        if (reader.wire_type() != protozero::pbf_wire_type::fixed64) {
                            throw format_exception{"Scaling base has wrong protobuf type"};
                        }
                        m_base = reader.get_double();
                        break;
                    default:
                        reader.skip();
                }
            }
        }

        /// Get the offset value of this scaling.
        constexpr int64_t offset() const noexcept {
            return m_offset;
        }

        /// Get the multiplier value of this scaling.
        constexpr double multiplier() const noexcept {
            return m_multiplier;
        }

        /// Get the base value of this scaling.
        constexpr double base() const noexcept {
            return m_base;
        }

        /**
         * Encode a value according to the scaling.
         */
        constexpr int64_t encode64(double value) const noexcept {
            return static_cast<int64_t>((value - m_base) / m_multiplier) - m_offset;
        }

        /**
         * Encode a value according to the scaling.
         *
         * The result value is converted into a 32bit integer.
         */
        constexpr int32_t encode32(double value) const noexcept {
            return static_cast<int32_t>(static_cast<int64_t>((value - m_base) / m_multiplier) - m_offset);
        }

        /// Decode a value according to the scaling.
        constexpr double decode(int64_t value) const noexcept {
            return m_base + m_multiplier * (static_cast<double>(value + m_offset));
        }

        /// Has this scaling the default values?
        bool is_default() const noexcept {
            return m_offset == 0 && m_multiplier == 1.0 && m_base == 0.0;
        }

        /**
         * The maximum size a scaling message can have "on the wire". Used to
         * allocate a buffer of the right size.
         */
        constexpr static std::size_t max_message_size() noexcept {
            return protozero::max_varint_length /* offset (varint) */ +
                   sizeof(double) /* multiplier (double) */ +
                   sizeof(double) /* base (double) */ +
                   3 /* types for offset, multiplier, and base */;
        }

        /// Serialize scaling into protobuf message.
        template <typename TBuffer>
        void serialize(TBuffer& buffer) const {
            protozero::basic_pbf_builder<TBuffer, detail::pbf_scaling> pbf_scaling{buffer};

            if (m_offset != 0) {
                pbf_scaling.add_sint64(detail::pbf_scaling::offset, m_offset);
            }
            if (m_multiplier != 1.0) {
                pbf_scaling.add_double(detail::pbf_scaling::multiplier, m_multiplier);
            }
            if (m_base != 0.0) {
                pbf_scaling.add_double(detail::pbf_scaling::base, m_base);
            }
        }

        /// Scalings are the same if all values are the same.
        constexpr bool operator==(const scaling& other) const noexcept {
            return m_offset == other.m_offset &&
                   m_multiplier == other.m_multiplier &&
                   m_base == other.m_base;
        }

        /// Scalings are different if they are not the same.
        constexpr bool operator!=(const scaling& other) const noexcept {
            return !(*this == other);
        }

    }; // class scaling

} // namespace vtzero

#endif // VTZERO_SCALING_HPP
