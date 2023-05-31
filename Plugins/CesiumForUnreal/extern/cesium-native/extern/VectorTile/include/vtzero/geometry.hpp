#ifndef VTZERO_GEOMETRY_HPP
#define VTZERO_GEOMETRY_HPP

/*****************************************************************************

vtzero - Tiny and fast vector tile decoder and encoder in C++.

This file is from https://github.com/mapbox/vtzero where you can find more
documentation.

*****************************************************************************/

/**
 * @file geometry.hpp
 *
 * @brief Contains classes and functions related to geometry handling.
 */

#include "exception.hpp"
#include "types.hpp"
#include <protozero/pbf_reader.hpp>

#include <cstdint>
#include <limits>
#include <utility>
#include <memory>
#include <vector>

namespace vtzero {

    /// A simple point class
    struct point {

        /// X coordinate
        int32_t x = 0;

        /// Y coordinate
        int32_t y = 0;

        /// Default construct to 0 coordinates
        constexpr point() noexcept = default;

        /// Constructor
        constexpr point(int32_t x_, int32_t y_) noexcept :
            x(x_),
            y(y_) {
        }

    }; // struct point

    struct line {
        typedef  std::vector<vtzero::point> points;
        points _pts;
    };


    /**
     * Type of a polygon ring. This can either be "outer", "inner", or
     * "invalid". Invalid is used when the area of the ring is 0.
     */
    enum class ring_type {
        outer = 0,
        inner = 1,
        invalid = 2
    }; // enum class ring_type

    struct ring
    {
        ring_type _t;
        line      _l;
    };

    struct polygon{
        typedef std::vector<ring> rings;
        rings _rs;
    };


    class geom_item
    {
    public:

        virtual vtzero::GeomType type()const = 0;
        
        virtual void begin(uint32_t count) = 0; 

        virtual void setpoint(const vtzero::point &pt) = 0;

        virtual void end(vtzero::ring_type type = vtzero::ring_type::invalid) = 0;
         
        virtual const void* data()const {
            assert(NULL);
            return nullptr;
        }
        
    };
    typedef std::shared_ptr<geom_item> GeoItemPtr;

    template<typename T>
    static const T& GetGeoData(GeoItemPtr ptr)
    {
        assert(ptr);
        return *reinterpret_cast<const T*>(ptr->data()); 
    } 
    #define SETGEOMVALUE(V,T,G) const T &V = vtzero::GetGeoData<T>(G)
    class geom_point : public geom_item
    {
    public:
        virtual vtzero::GeomType type()const final
        {
            return vtzero::GeomType::POINT;
        }
    };
    class geom_line : public geom_item
    {
    public:

        virtual vtzero::GeomType type()const final
        {
            return vtzero::GeomType::LINESTRING;
        }
    };
    class geom_polygon : public geom_item
    {
    public:
        virtual vtzero::GeomType type()const final
        {
            return vtzero::GeomType::POLYGON;
        }
    };

    
    /**
     * Helper function to create a point from any type that has members x
     * and y.
     *
     * If your point type doesn't have members x any y, you can overload this
     * function for your type and it will be used by vtzero.
     */
    template <typename TPoint>
    point create_vtzero_point(const TPoint& p) noexcept {
        return {p.x, p.y};
    }

    /// Points are equal if their coordinates are
    inline constexpr bool operator==(const point a, const point b) noexcept {
        return a.x == b.x && a.y == b.y;
    }

    /// Points are not equal if their coordinates aren't
    inline constexpr bool operator!=(const point a, const point b) noexcept {
        return !(a == b);
    }

    namespace detail {

        /// The command id type as specified in the vector tile spec
        enum class CommandId : uint32_t {
            MOVE_TO = 1,
            LINE_TO = 2,
            CLOSE_PATH = 7
        };

        inline constexpr uint32_t command_integer(CommandId id, const uint32_t count) noexcept {
            return (static_cast<uint32_t>(id) & 0x7U) | (count << 3U);
        }

        inline constexpr uint32_t command_move_to(const uint32_t count) noexcept {
            return command_integer(CommandId::MOVE_TO, count);
        }

        inline constexpr uint32_t command_line_to(const uint32_t count) noexcept {
            return command_integer(CommandId::LINE_TO, count);
        }

        inline constexpr uint32_t command_close_path() noexcept {
            return command_integer(CommandId::CLOSE_PATH, 1);
        }

        inline constexpr uint32_t get_command_id(const uint32_t command_integer) noexcept {
            return command_integer & 0x7U;
        }

        inline constexpr uint32_t get_command_count(const uint32_t command_integer) noexcept {
            return command_integer >> 3U;
        }

        // The maximum value for the command count according to the spec.
        inline constexpr uint32_t max_command_count() noexcept {
            return get_command_count(std::numeric_limits<uint32_t>::max());
        }

        inline constexpr int64_t det(const point a, const point b) noexcept {
            return static_cast<int64_t>(a.x) * static_cast<int64_t>(b.y) -
                   static_cast<int64_t>(b.x) * static_cast<int64_t>(a.y);
        }

        /**
         * Decode a geometry as specified in spec 4.3 from a sequence of 32 bit
         * unsigned integers. This templated base class can be instantiated
         * with a different iterator type for testing than for normal use.
         */
        template <typename TIterator>
        class geometry_decoder {

        public:

            using iterator_type = TIterator;

        private:

            iterator_type m_it;
            iterator_type m_end;

            point m_cursor{0, 0};

            // maximum value for m_count before we throw an exception
            uint32_t m_max_count;

            /**
             * The current count value is set from the CommandInteger and
             * then counted down with each next_point() call. So it must be
             * greater than 0 when next_point() is called and 0 when
             * next_command() is called.
             */
            uint32_t m_count = 0;

        public:

            geometry_decoder(iterator_type begin, iterator_type end, std::size_t max) :
                m_it(begin),
                m_end(end),
                m_max_count(static_cast<uint32_t>(max)) {
                vtzero_assert(max <= detail::max_command_count());
            }

            uint32_t count() const noexcept {
                return m_count;
            }

            bool done() const noexcept {
                return m_it == m_end;
            }

            bool next_command(const CommandId expected_command_id) {
                vtzero_assert(m_count == 0);

                if (m_it == m_end) {
                    return false;
                }

                const auto command_id = get_command_id(*m_it);
                if(command_id == 0)
                    return false;
                if (command_id != static_cast<uint32_t>(expected_command_id)) {
                    throw geometry_exception{std::string{"expected command "} +
                                             std::to_string(static_cast<uint32_t>(expected_command_id)) +
                                             " but got " +
                                             std::to_string(command_id)};
                }

                if (expected_command_id == CommandId::CLOSE_PATH) {
                    // spec 4.3.3.3 "A ClosePath command MUST have a command count of 1"
                    if (get_command_count(*m_it) != 1) {
                        throw geometry_exception{"ClosePath command count is not 1"};
                    }
                } else {
                    m_count = get_command_count(*m_it);
                    if (m_count > m_max_count) {
                        throw geometry_exception{"count too large"};
                    }
                }

                ++m_it;

                return true;
            }

            point next_point() {
                vtzero_assert(m_count > 0);

                if (m_it == m_end || std::next(m_it) == m_end) {
                    throw geometry_exception{"too few points in geometry"};
                }

                // spec 4.3.2 "A ParameterInteger is zigzag encoded"
                int64_t x = protozero::decode_zigzag32(*m_it++);
                int64_t y = protozero::decode_zigzag32(*m_it++);

                // x and y are int64_t so this addition can never overflow
                x += m_cursor.x;
                y += m_cursor.y;

                // The cast is okay, because a valid vector tile can never
                // contain values that would overflow here and we don't care
                // what happens to invalid tiles here.
                m_cursor.x = static_cast<int32_t>(x);
                m_cursor.y = static_cast<int32_t>(y);

                --m_count;

                return m_cursor;
            }

            void decode_point(GeoItemPtr geom_handler) {
                // spec 4.3.4.2 "MUST consist of a single MoveTo command"
                if (!next_command(CommandId::MOVE_TO)) {
                    throw geometry_exception{"expected MoveTo command (spec 4.3.4.2)"};
                }

                // spec 4.3.4.2 "command count greater than 0"
                if (count() == 0) {
                    throw geometry_exception{"MoveTo command count is zero (spec 4.3.4.2)"};
                }

                geom_handler->begin(count());
                while (count() > 0) {
                    geom_handler->setpoint(next_point());
                }

                // spec 4.3.4.2 "MUST consist of of a single ... command"
                if (!done()) {
                    throw geometry_exception{"additional data after end of geometry (spec 4.3.4.2)"};
                }

                geom_handler->end();
            }

            void decode_linestring(GeoItemPtr geom_handler) {
                // spec 4.3.4.3 "1. A MoveTo command"
                while (next_command(CommandId::MOVE_TO)) {
                    // spec 4.3.4.3 "with a command count of 1"
                    if (count() != 1) {
                        throw geometry_exception{"MoveTo command count is not 1 (spec 4.3.4.3)"};
                    }

                    const auto first_point = next_point();

                    // spec 4.3.4.3 "2. A LineTo command"
                    if (!next_command(CommandId::LINE_TO)) {
                        throw geometry_exception{"expected LineTo command (spec 4.3.4.3)"};
                    }

                    // spec 4.3.4.3 "with a command count greater than 0"
                    if (count() == 0) {
                        throw geometry_exception{"LineTo command count is zero (spec 4.3.4.3)"};
                    }

                    geom_handler->begin(count() + 1);

                    geom_handler->setpoint(first_point);
                    while (count() > 0) {
                        geom_handler->setpoint(next_point());
                    }

                    geom_handler->end();
                }
            }

            void decode_polygon(GeoItemPtr geom_handler) {
                // spec 4.3.4.4 "1. A MoveTo command"
                while (next_command(CommandId::MOVE_TO)) {
                    // spec 4.3.4.4 "with a command count of 1"
                    if (count() != 1) {
                        throw geometry_exception{"MoveTo command count is not 1 (spec 4.3.4.4)"};
                    }

                    int64_t sum = 0;
                    const point start_point = next_point();
                    point last_point = start_point;

                    // spec 4.3.4.4 "2. A LineTo command"
                    if (!next_command(CommandId::LINE_TO)) {
                        throw geometry_exception{"expected LineTo command (spec 4.3.4.4)"};
                    }

                    geom_handler->begin(count() + 2);

                    geom_handler->setpoint(start_point);

                    while (count() > 0) {
                        const point p = next_point();
                        sum += detail::det(last_point, p);
                        last_point = p;
                        geom_handler->setpoint(p);
                    }

                    // spec 4.3.4.4 "3. A ClosePath command"
                    if (!next_command(CommandId::CLOSE_PATH)) {
                        throw geometry_exception{"expected ClosePath command (4.3.4.4)"};
                    }
                    //  E|XnYn+1 - YnXn+1|  check if clockwise
                    sum += detail::det(last_point, start_point);

                    geom_handler->setpoint(start_point);

                    geom_handler->end(sum > 0 ? ring_type::outer :
                                       sum < 0 ? ring_type::inner : ring_type::invalid);
                }
            }

        }; // class geometry_decoder

    } // namespace detail

    /**
     * Decode a point geometry.
     *
     * @param geometry The geometry as returned by feature.geometry().
     * @param geom_handler An object of TGeomHandler.
     * @throws geometry_error If there is a problem with the geometry.
     * @pre Geometry must be a point geometry.
     */
    static void decode_point_geometry(const geometry& geometry, GeoItemPtr geom_handler) {
        vtzero_assert(geometry.type() == GeomType::POINT);
        detail::geometry_decoder<decltype(geometry.begin())> decoder{geometry.begin(), geometry.end(), geometry.data().size() / 2};
        return decoder.decode_point(geom_handler);
    }

    /**
     * Decode a linestring geometry.
     *
     * @param geometry The geometry as returned by feature.geometry().
     * @param geom_handler An object of TGeomHandler.
     * @returns whatever geom_handler.result() returns if that function exists,
     *          void otherwise
     * @throws geometry_error If there is a problem with the geometry.
     * @pre Geometry must be a linestring geometry.
     */
    static void decode_linestring_geometry(const geometry& geometry, GeoItemPtr geom_handler) {
        vtzero_assert(geometry.type() == GeomType::LINESTRING);
        detail::geometry_decoder<decltype(geometry.begin())> decoder{geometry.begin(), geometry.end(), geometry.data().size() / 2};
        return decoder.decode_linestring(geom_handler);
    }

    /**
     * Decode a polygon geometry.
     *
     * @param geometry The geometry as returned by feature.geometry().
     * @param geom_handler An object of TGeomHandler.
     * @returns whatever geom_handler.result() returns if that function exists,
     *          void otherwise
     * @throws geometry_error If there is a problem with the geometry.
     * @pre Geometry must be a polygon geometry.
     */
    static void decode_polygon_geometry(const geometry& geometry, GeoItemPtr geom_handler) {
        vtzero_assert(geometry.type() == GeomType::POLYGON);
        detail::geometry_decoder<decltype(geometry.begin())> decoder{geometry.begin(), geometry.end(), geometry.data().size() / 2};
        return decoder.decode_polygon(geom_handler);
    }

    /**
     * Decode a geometry.
     *
     * @param geometry The geometry as returned by feature.geometry().
     * @param geom_handler An object of TGeomHandler.
     * @returns whatever geom_handler.result() returns if that function exists,
     *          void otherwise
     * @throws geometry_error If the geometry has type UNKNOWN of if there is
     *                        a problem with the geometry.
     */
    static void decode_geometry(const geometry& geometry, GeoItemPtr geom_handler) {
        detail::geometry_decoder<decltype(geometry.begin())> decoder{geometry.begin(), geometry.end(), geometry.data().size() / 2};
        switch (geometry.type()) {
            case GeomType::POINT:
                return decoder.decode_point(geom_handler);
            case GeomType::LINESTRING:
                return decoder.decode_linestring(geom_handler);
            case GeomType::POLYGON:
                return decoder.decode_polygon(geom_handler);
            default:
                break;
        }
        throw geometry_exception{"unknown geometry type"};
    }

} // namespace vtzero

#endif // VTZERO_GEOMETRY_HPP
