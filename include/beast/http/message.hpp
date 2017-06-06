//
// Copyright (c) 2013-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BEAST_HTTP_MESSAGE_HPP
#define BEAST_HTTP_MESSAGE_HPP

#include <beast/config.hpp>
#include <beast/http/connection.hpp>
#include <beast/http/fields.hpp>
#include <beast/http/verb.hpp>
#include <beast/http/status.hpp>
#include <beast/http/type_traits.hpp>
#include <beast/core/string_view.hpp>
#include <beast/core/detail/integer_sequence.hpp>
#include <boost/optional.hpp>
#include <boost/throw_exception.hpp>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

namespace beast {
namespace http {

/** A container for an HTTP request or response header.

    A header includes the Start Line and Fields.

    Some use-cases:

    @li When the message has no body, such as a response to a HEAD request.

    @li When the caller wishes to defer instantiation of the body.

    @li Invoke algorithms which operate on the header only.
*/
#if BEAST_DOXYGEN
template<bool isRequest, class Fields = fields>
struct header

#else
template<bool isRequest, class Fields = fields>
struct header;

template<class Fields>
struct header<true, Fields> : Fields
#endif
{
    /// Indicates if the header is a request or response.
    using is_request = std::true_type;

    /// The type representing the fields.
    using fields_type = Fields;

    /** The HTTP-version.

        This holds both the major and minor version numbers,
        using these formulas:
        @code
            int major = version / 10;
            int minor = version % 10;
        @endcode
    */
    int version;

    /// Default constructor
    header() = default;

    /// Move constructor
    header(header&&) = default;

    /// Copy constructor
    header(header const&) = default;

    /// Move assignment
    header& operator=(header&&) = default;

    /// Copy assignment
    header& operator=(header const&) = default;

    /** Construct the header.

        All arguments are forwarded to the constructor
        of the `fields` member.

        @note This constructor participates in overload resolution
        if and only if the first parameter is not convertible to
        `header`.
    */
#if BEAST_DOXYGEN
    template<class... Args>
    explicit
    header(Args&&... args);

#else
    template<class Arg1, class... ArgN,
        class = typename std::enable_if<
            (sizeof...(ArgN) > 0) || ! std::is_convertible<
                typename std::decay<Arg1>::type,
                    header>::value>::type>
    explicit
    header(Arg1&& arg1, ArgN&&... argn)
        : Fields(std::forward<Arg1>(arg1),
            std::forward<ArgN>(argn)...)
    {
    }

    /** Return the request-method verb.

        If the request-method is not one of the recognized verbs,
        @ref verb::unknown is returned. Callers may use @ref method_string
        to retrieve the exact text.

        @note This function is only available when `isRequest == true`.

        @see @ref method_string
    */
    verb
    method() const
    {
        return method_;
    }

    /** Set the request-method verb.

        This function will set the method for requests to a known verb.

        @param v The request method verb to set.
        This may not be @ref verb::unknown.

        @throw std::invalid_argument when `v == verb::unknown`.
    */
    void
    method(verb v)
    {
        set_method(v);
    }

    /** Return the request-method string.

        @note This function is only available when `isRequest == true`.

        @see @ref method
    */
    string_view
    method_string() const
    {
        return get_method_string();
    }

    /** Set the request-method string.

        This function will set the method for requests to a verb
        if the string matches a known verb, otherwise it will
        store a copy of the passed string as the method.

        @param s A string representing the request-method.

        @note This function is only available when `isRequest == true`.
    */
    void
    method(string_view s)
    {
        set_method(s);
    }

    /** Returns the request-target string.

        @note This function is only available when `isRequest == true`.
    */
    string_view
    target() const
    {
        return this->target_impl();
    }

    /** Set the request-target string.

        @param s A string representing the request-target.

        @note This function is only available when `isRequest == true`.
    */
    void
    target(string_view s)
    {
        this->target_impl(s);
    }

private:
    template<bool, class, class>
    friend struct message;

    template<class T>
    friend
    void
    swap(header<true, T>& m1, header<true, T>& m2);

    template<class = void>
    string_view
    get_method_string() const;

    template<class = void>
    void
    set_method(verb v);

    template<class = void>
    void
    set_method(string_view v);

    verb method_ = verb::unknown;
};

/** A container for an HTTP request or response header.

    A header includes the Start Line and Fields.

    Some use-cases:

    @li When the message has no body, such as a response to a HEAD request.

    @li When the caller wishes to defer instantiation of the body.

    @li Invoke algorithms which operate on the header only.
*/
template<class Fields>
struct header<false, Fields> : Fields
{
    /// Indicates if the header is a request or response.
    using is_request = std::false_type;

    /// The type representing the fields.
    using fields_type = Fields;

    /** The HTTP version.

        This holds both the major and minor version numbers,
        using these formulas:
        @code
            major = version / 10;
            minor = version % 10;
        @endcode
    */
    int version;

    /// Default constructor.
    header() = default;

    /// Move constructor
    header(header&&) = default;

    /// Copy constructor
    header(header const&) = default;

    /// Move assignment
    header& operator=(header&&) = default;

    /// Copy assignment
    header& operator=(header const&) = default;

    /** Construct the header.

        All arguments are forwarded to the constructor
        of the `fields` member.

        @note This constructor participates in overload resolution
        if and only if the first parameter is not convertible to
        `header`.
    */
    template<class Arg1, class... ArgN,
        class = typename std::enable_if<
            (sizeof...(ArgN) > 0) || ! std::is_convertible<
                typename std::decay<Arg1>::type,
                    header>::value>::type>
    explicit
    header(Arg1&& arg1, ArgN&&... argn)
        : Fields(std::forward<Arg1>(arg1),
            std::forward<ArgN>(argn)...)
    {
    }
#endif

    /** The response status-code result.

        If the actual status code is not a known code, this
        function returns @ref status::unknown. Use @ref result_int
        to return the raw status code as a number.

        @note This member is only available when `isRequest == false`.
    */
    status
    result() const
    {
        return int_to_status(
            static_cast<int>(result_));
    }

    /** Set the response status-code result.

        @param v The code to set.

        @note This member is only available when `isRequest == false`.
    */
    void
    result(status v)
    {
        result_ = v;
    }

    /** Set the raw status-code result as an integer.

        This sets the status code to the exact number passed in.
        If the number does not correspond to one of the known
        status codes, the function @ref result will return
        @ref status::unknown. Use @ref result_int to obtain the
        original raw status-code.

        @param v The status-code integer to set.
    */
    void
    result(int v)
    {
        result_ = static_cast<status>(v);
    }

    /** The response status-code result expressed as an integer.

        This returns the raw status code as an integer, even
        when that code is not in the list of known status codes.

        @note This member is only available when `isRequest == false`.
    */
    int
    result_int() const
    {
        return static_cast<int>(result_);
    }


    /** Return the response reason-phrase.

        The reason-phrase is obsolete as of rfc7230.

        @note This function is only available when `isRequest == false`.
    */
    string_view
    reason() const
    {
        return get_reason();
    }

    /** Set the response reason-phrase (deprecated)

        This function sets a custom reason-phrase to a copy of
        the string passed in. Normally it is not necessary to set
        the reason phrase on an outgoing response object; the
        implementation will automatically use the standard reason
        text for the corresponding status code.

        To clear a previously set custom phrase, pass an empty
        string. This will restore the default standard reason text
        based on the status code used when serializing.

        The reason-phrase is obsolete as of rfc7230.

        @param s The string to use for the reason-phrase.

        @note This function is only available when `isRequest == false`.
    */
    void
    reason(string_view s)
    {
        this->reason_impl(s);
    }
   
private:
    template<bool, class, class>
    friend struct message;

    template<class T>
    friend
    void
    swap(header<false, T>& m1, header<false, T>& m2);

    template<class = void>
    string_view
    get_reason() const;

    status result_;
};

/** A container for a complete HTTP message.

    A message can be a request or response, depending on the
    `isRequest` template argument value. Requests and responses
    have different types; functions may be overloaded based on
    the type if desired.

    The `Body` template argument type determines the model used
    to read or write the content body of the message.

    @tparam isRequest `true` if this represents a request,
    or `false` if this represents a response. Some class data
    members are conditionally present depending on this value.

    @tparam Body A type meeting the requirements of Body.

    @tparam Fields The type of container used to hold the
    field value pairs.
*/
template<bool isRequest, class Body, class Fields = fields>
struct message : header<isRequest, Fields>
{
    /// The base class used to hold the header portion of the message.
    using header_type = header<isRequest, Fields>;

    /** The type providing the body traits.

        The @ref message::body member will be of type `body_type::value_type`.
    */
    using body_type = Body;

    /// A value representing the body.
    typename Body::value_type body;

    /// Default constructor
    message() = default;

    /// Move constructor
    message(message&&) = default;

    /// Copy constructor
    message(message const&) = default;

    /// Move assignment
    message& operator=(message&&) = default;

    /// Copy assignment
    message& operator=(message const&) = default;

    /** Construct a message from a header.

        Additional arguments, if any, are forwarded to
        the constructor of the body member.
    */
    template<class... Args>
    explicit
    message(header_type&& base, Args&&... args)
        : header_type(std::move(base))
        , body(std::forward<Args>(args)...)
    {
    }

    /** Construct a message from a header.

        Additional arguments, if any, are forwarded to
        the constructor of the body member.
    */
    template<class... Args>
    explicit
    message(header_type const& base, Args&&... args)
        : header_type(base)
        , body(std::forward<Args>(args)...)
    {
    }

    /** Construct a message.

        @param u An argument forwarded to the body constructor.

        @note This constructor participates in overload resolution
        only if `u` is not convertible to `header_type`.
    */
    template<class U
#if ! BEAST_DOXYGEN
        , class = typename std::enable_if<
            ! std::is_convertible<typename
                std::decay<U>::type, header_type>::value>::type
#endif
    >
    explicit
    message(U&& u)
        : body(std::forward<U>(u))
    {
    }

    /** Construct a message.

        @param u An argument forwarded to the body constructor.

        @param v An argument forwarded to the fields constructor.

        @note This constructor participates in overload resolution
        only if `u` is not convertible to `header_type`.
    */
    template<class U, class V
#if ! BEAST_DOXYGEN
        ,class = typename std::enable_if<! std::is_convertible<
            typename std::decay<U>::type, header_type>::value>::type
#endif
    >
    message(U&& u, V&& v)
        : header_type(std::forward<V>(v))
        , body(std::forward<U>(u))
    {
    }

    /** Construct a message.

        @param un A tuple forwarded as a parameter pack to the body constructor.
    */
    template<class... Un>
    message(std::piecewise_construct_t, std::tuple<Un...> un)
        : message(std::piecewise_construct, un,
            beast::detail::make_index_sequence<sizeof...(Un)>{})
    {
    }

    /** Construct a message.

        @param un A tuple forwarded as a parameter pack to the body constructor.

        @param vn A tuple forwarded as a parameter pack to the fields constructor.
    */
    template<class... Un, class... Vn>
    message(std::piecewise_construct_t,
            std::tuple<Un...>&& un, std::tuple<Vn...>&& vn)
        : message(std::piecewise_construct, un, vn,
            beast::detail::make_index_sequence<sizeof...(Un)>{},
            beast::detail::make_index_sequence<sizeof...(Vn)>{})
    {
    }

    /** Returns `true` if Transfer-Encoding is present, and chunked appears last.
    */
    bool
    chunked() const;

    /** Returns the payload size of the body in octets if possible.

        This function invokes the @b Body algorithm to measure
        the number of octets in the serialized body container. If
        there is no body, this will return zero. Otherwise, if the
        body exists but is not known ahead of time, `boost::none`
        is returned (usually indicating that a chunked Transfer-Encoding
        will be used).

        @note The value of the Content-Length field in the message
        is not inspected.
    */
    boost::optional<std::uint64_t>
    size() const;

    /** Set the Content-Length field.

        The value of the Content-Length field will be unconditionally
        set to the specified number of octets.

        @para n The number of octets to set for the Content-Length field.
    */
    void
    content_length(std::uint64_t n);

    /** Prepare some fields automatically.

        This function will adjust the Connection, Content-Length
        and Transfer-Encoding, fields of the message based on the
        properties of the body and the options passed in.

        @par Example
        @code
        request<empty_body> req;
        req.version = 11;
        req.method(verb::upgrade);
        req.target("/");
        req.insert(field::user_agent, "Beast");
        req.prepare(connection::close, connection::upgrade);
        @endcode

        @param args An list of zero or more options to use.

        @throw std::invalid_argument if the values of certain
        fields detectably violate the semantic requirements of HTTP.

        @note Undefined behavior if called more than once.

        @see @ref connection
    */
    template<class... Args>
    void
    prepare(Args const&... args);

private:
    static_assert(is_body<Body>::value,
        "Body requirements not met");

    template<class... Un, size_t... IUn>
    message(std::piecewise_construct_t,
        std::tuple<Un...>& tu,
            beast::detail::index_sequence<IUn...>)
        : body(std::forward<Un>(std::get<IUn>(tu))...)
    {
    }

    template<class... Un, class... Vn,
        std::size_t... IUn, std::size_t... IVn>
    message(std::piecewise_construct_t,
            std::tuple<Un...>& tu, std::tuple<Vn...>& tv,
                beast::detail::index_sequence<IUn...>,
                    beast::detail::index_sequence<IVn...>)
        : header_type(std::forward<Vn>(std::get<IVn>(tv))...)
        , body(std::forward<Un>(std::get<IUn>(tu))...)
    {
    }

    boost::optional<std::uint64_t>
    size(std::true_type) const
    {
        return Body::size(*this);
    }

    boost::optional<std::uint64_t>
    size(std::false_type) const
    {
        return boost::none;
    }

    template<class Arg, class... Args>
    void
    prepare_opt(unsigned&, Arg const&, Args const&...);

    void
    prepare_opt(unsigned&)
    {
    }

    void
    prepare_opt(unsigned&, close_t);

    void
    prepare_opt(unsigned&, keep_alive_t);

    void
    prepare_opt(unsigned&, upgrade_t);

    void
    prepare_payload(std::true_type);

    void
    prepare_payload(std::false_type);
};

/// A typical HTTP request
template<class Body, class Fields = fields>
using request = message<true, Body, Fields>;

/// A typical HTTP response
template<class Body, class Fields = fields>
using response = message<false, Body, Fields>;

//------------------------------------------------------------------------------

#if BEAST_DOXYGEN
/** Swap two header objects.

    @par Requirements
    `Fields` is @b Swappable.
*/
template<bool isRequest, class Fields>
void
swap(
    header<isRequest, Fields>& m1,
    header<isRequest, Fields>& m2);
#endif

/** Swap two message objects.

    @par Requirements:
    `Body::value_type` and `Fields` are @b Swappable.
*/
template<bool isRequest, class Body, class Fields>
void
swap(
    message<isRequest, Body, Fields>& m1,
    message<isRequest, Body, Fields>& m2);

//------------------------------------------------------------------------------

/** Returns `true` if the HTTP/1 message indicates a keep alive.

    Undefined behavior if version is greater than 11.
*/
template<bool isRequest, class Fields>
bool
is_keep_alive(header<isRequest, Fields> const& msg);

/** Returns `true` if the HTTP/1 message indicates an Upgrade request or response.

    Undefined behavior if version is greater than 11.
*/
template<bool isRequest, class Fields>
bool
is_upgrade(header<isRequest, Fields> const& msg);

} // http
} // beast

#include <beast/http/impl/message.ipp>

#endif
