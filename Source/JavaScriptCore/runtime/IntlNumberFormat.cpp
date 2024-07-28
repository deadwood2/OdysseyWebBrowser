/*
 * Copyright (C) 2015 Andy VanWagoner (thetalecrafter@gmail.com)
 * Copyright (C) 2016 Sukolsak Sakshuwong (sukolsak@gmail.com)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "IntlNumberFormat.h"

#if ENABLE(INTL)

#include "Error.h"
#include "IdentifierInlines.h"
#include "IntlNumberFormatConstructor.h"
#include "IntlObject.h"
#include "JSBoundFunction.h"
#include "JSCJSValueInlines.h"
#include "JSCellInlines.h"
#include "ObjectConstructor.h"
#include "SlotVisitorInlines.h"
#include "StructureInlines.h"

namespace JSC {

const ClassInfo IntlNumberFormat::s_info = { "Object", &Base::s_info, 0, CREATE_METHOD_TABLE(IntlNumberFormat) };

static const char* const relevantExtensionKeys[1] = { "nu" };

IntlNumberFormat* IntlNumberFormat::create(VM& vm, IntlNumberFormatConstructor* constructor)
{
    IntlNumberFormat* format = new (NotNull, allocateCell<IntlNumberFormat>(vm.heap)) IntlNumberFormat(vm, constructor->numberFormatStructure());
    format->finishCreation(vm);
    return format;
}

Structure* IntlNumberFormat::createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
{
    return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), info());
}

IntlNumberFormat::IntlNumberFormat(VM& vm, Structure* structure)
    : JSDestructibleObject(vm, structure)
{
}

void IntlNumberFormat::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    ASSERT(inherits(info()));
}

void IntlNumberFormat::destroy(JSCell* cell)
{
    static_cast<IntlNumberFormat*>(cell)->IntlNumberFormat::~IntlNumberFormat();
}

void IntlNumberFormat::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    IntlNumberFormat* thisObject = jsCast<IntlNumberFormat*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());

    Base::visitChildren(thisObject, visitor);

    visitor.append(&thisObject->m_boundFormat);
}

static Vector<String> localeData(const String& locale, size_t keyIndex)
{
    // 9.1 Internal slots of Service Constructors & 11.2.3 Internal slots (ECMA-402 2.0)
    ASSERT_UNUSED(keyIndex, !keyIndex); // The index of the extension key "nu" in relevantExtensionKeys is 0.
    return numberingSystemsForLocale(locale);
}

static inline unsigned computeCurrencySortKey(const String& currency)
{
    ASSERT(currency.length() == 3);
    ASSERT(currency.isAllSpecialCharacters<isASCIIUpper>());
    return (currency[0] << 16) + (currency[1] << 8) + currency[2];
}

static inline unsigned computeCurrencySortKey(const char* currency)
{
    ASSERT(strlen(currency) == 3);
    ASSERT(isAllSpecialCharacters<isASCIIUpper>(currency, 3));
    return (currency[0] << 16) + (currency[1] << 8) + currency[2];
}

static unsigned extractCurrencySortKey(std::pair<const char*, unsigned>* currencyMinorUnit)
{
    return computeCurrencySortKey(currencyMinorUnit->first);
}

static unsigned computeCurrencyDigits(const String& currency)
{
    // 11.1.1 The abstract operation CurrencyDigits (currency)
    // "If the ISO 4217 currency and funds code list contains currency as an alphabetic code,
    // then return the minor unit value corresponding to the currency from the list; else return 2.
    std::pair<const char*, unsigned> currencyMinorUnits[] = {
        { "BHD", 3 },
        { "BIF", 0 },
        { "BYR", 0 },
        { "CLF", 4 },
        { "CLP", 0 },
        { "DJF", 0 },
        { "GNF", 0 },
        { "IQD", 3 },
        { "ISK", 0 },
        { "JOD", 3 },
        { "JPY", 0 },
        { "KMF", 0 },
        { "KRW", 0 },
        { "KWD", 3 },
        { "LYD", 3 },
        { "OMR", 3 },
        { "PYG", 0 },
        { "RWF", 0 },
        { "TND", 3 },
        { "UGX", 0 },
        { "UYI", 0 },
        { "VND", 0 },
        { "VUV", 0 },
        { "XAF", 0 },
        { "XOF", 0 },
        { "XPF", 0 }
    };
    auto* currencyMinorUnit = tryBinarySearch<std::pair<const char*, unsigned>>(currencyMinorUnits, WTF_ARRAY_LENGTH(currencyMinorUnits), computeCurrencySortKey(currency), extractCurrencySortKey);
    if (currencyMinorUnit)
        return currencyMinorUnit->second;
    return 2;
}

void IntlNumberFormat::initializeNumberFormat(ExecState& state, JSValue locales, JSValue optionsValue)
{
    // 11.1.1 InitializeNumberFormat (numberFormat, locales, options) (ECMA-402 2.0)
    VM& vm = state.vm();

    // 1. If numberFormat has an [[initializedIntlObject]] internal slot with value true, throw a TypeError exception.
    // 2. Set numberFormat.[[initializedIntlObject]] to true.

    // 3. Let requestedLocales be CanonicalizeLocaleList(locales).
    auto requestedLocales = canonicalizeLocaleList(state, locales);
    // 4. ReturnIfAbrupt(requestedLocales).
    if (state.hadException())
        return;

    // 5. If options is undefined, then
    JSObject* options;
    if (optionsValue.isUndefined()) {
        // a. Let options be ObjectCreate(%ObjectPrototype%).
        options = constructEmptyObject(&state);
    } else { // 6. Else
        // a. Let options be ToObject(options).
        options = optionsValue.toObject(&state);
        // b. ReturnIfAbrupt(options).
        if (state.hadException())
            return;
    }

    // 7. Let opt be a new Record.
    HashMap<String, String> opt;

    // 8. Let matcher be GetOption(options, "localeMatcher", "string", «"lookup", "best fit"», "best fit").
    String matcher = intlStringOption(state, options, state.vm().propertyNames->localeMatcher, { "lookup", "best fit" }, "localeMatcher must be either \"lookup\" or \"best fit\"", "best fit");
    // 9. ReturnIfAbrupt(matcher).
    if (state.hadException())
        return;
    // 10. Set opt.[[localeMatcher]] to matcher.
    opt.add(ASCIILiteral("localeMatcher"), matcher);

    // 11. Let localeData be %NumberFormat%.[[localeData]].
    // 12. Let r be ResolveLocale(%NumberFormat%.[[availableLocales]], requestedLocales, opt, %NumberFormat%.[[relevantExtensionKeys]], localeData).
    auto& availableLocales = state.callee()->globalObject()->intlNumberFormatAvailableLocales();
    auto result = resolveLocale(availableLocales, requestedLocales, opt, relevantExtensionKeys, WTF_ARRAY_LENGTH(relevantExtensionKeys), localeData);

    // 13. Set numberFormat.[[locale]] to the value of r.[[locale]].
    m_locale = result.get(ASCIILiteral("locale"));

    // 14. Set numberFormat.[[numberingSystem]] to the value of r.[[nu]].
    m_numberingSystem = result.get(ASCIILiteral("nu"));

    // 15. Let dataLocale be r.[[dataLocale]].

    // 16. Let s be GetOption(options, "style", "string", « "decimal", "percent", "currency"», "decimal").
    String styleString = intlStringOption(state, options, Identifier::fromString(&vm, "style"), { "decimal", "percent", "currency" }, "style must be either \"decimal\", \"percent\", or \"currency\"", "decimal");
    // 17. ReturnIfAbrupt(s).
    if (state.hadException())
        return;
    // 18. Set numberFormat.[[style]] to s.
    if (styleString == "decimal")
        m_style = Style::Decimal;
    else if (styleString == "percent")
        m_style = Style::Percent;
    else if (styleString == "currency")
        m_style = Style::Currency;
    else
        ASSERT_NOT_REACHED();

    // 19. Let c be GetOption(options, "currency", "string", undefined, undefined).
    String currency = intlStringOption(state, options, Identifier::fromString(&vm, "currency"), { }, nullptr, nullptr);
    // 20. ReturnIfAbrupt(c).
    if (state.hadException())
        return;
    // 21. If c is not undefined, then
    if (!currency.isNull()) {
        // a. If the result of IsWellFormedCurrencyCode(c), is false, then throw a RangeError exception.
        if (currency.length() != 3 || !currency.isAllSpecialCharacters<isASCIIAlpha>()) {
            state.vm().throwException(&state, createRangeError(&state, ASCIILiteral("currency is not a well-formed currency code")));
            return;
        }
    }

    unsigned currencyDigits;
    if (m_style == Style::Currency) {
        // 22. If s is "currency" and c is undefined, throw a TypeError exception.
        if (currency.isNull()) {
            throwTypeError(&state, ASCIILiteral("currency must be a string"));
            return;
        }

        // 23. If s is "currency", then
        // a. Let c be converting c to upper case as specified in 6.1.
        currency = currency.convertToASCIIUppercase();
        // b. Set numberFormat.[[currency]] to c.
        m_currency = currency;
        // c. Let cDigits be CurrencyDigits(c)
        currencyDigits = computeCurrencyDigits(currency);
    }

    // 24. Let cd be GetOption(options, "currencyDisplay", "string", «"code", "symbol", "name"», "symbol").
    String currencyDisplayString = intlStringOption(state, options, Identifier::fromString(&vm, "currencyDisplay"), { "code", "symbol", "name" }, "currencyDisplay must be either \"code\", \"symbol\", or \"name\"", "symbol");
    // 25. ReturnIfAbrupt(cd).
    if (state.hadException())
        return;
    // 26. If s is "currency", set numberFormat.[[currencyDisplay]] to cd.
    if (m_style == Style::Currency) {
        if (currencyDisplayString == "code")
            m_currencyDisplay = CurrencyDisplay::Code;
        else if (currencyDisplayString == "symbol")
            m_currencyDisplay = CurrencyDisplay::Symbol;
        else if (currencyDisplayString == "name")
            m_currencyDisplay = CurrencyDisplay::Name;
        else
            ASSERT_NOT_REACHED();
    }

    // 27. Let mnid be GetNumberOption(options, "minimumIntegerDigits", 1, 21, 1).
    // 28. ReturnIfAbrupt(mnid).
    // 29. Set numberFormat.[[minimumIntegerDigits]] to mnid.
    unsigned minimumIntegerDigits = intlNumberOption(state, options, Identifier::fromString(&vm, "minimumIntegerDigits"), 1, 21, 1);
    if (state.hadException())
        return;
    m_minimumIntegerDigits = minimumIntegerDigits;

    // 30. If s is "currency", let mnfdDefault be cDigits; else let mnfdDefault be 0.
    unsigned minimumFractionDigitsDefault = (m_style == Style::Currency) ? currencyDigits : 0;

    // 31. Let mnfd be GetNumberOption(options, "minimumFractionDigits", 0, 20, mnfdDefault).
    // 32. ReturnIfAbrupt(mnfd).
    // 33. Set numberFormat.[[minimumFractionDigits]] to mnfd.
    unsigned minimumFractionDigits = intlNumberOption(state, options, Identifier::fromString(&vm, "minimumFractionDigits"), 0, 20, minimumFractionDigitsDefault);
    if (state.hadException())
        return;
    m_minimumFractionDigits = minimumFractionDigits;

    // 34. If s is "currency", let mxfdDefault be max(mnfd, cDigits);
    unsigned maximumFractionDigitsDefault;
    if (m_style == Style::Currency)
        maximumFractionDigitsDefault = std::max(minimumFractionDigits, currencyDigits);
    else if (m_style == Style::Percent) // else if s is "percent", let mxfdDefault be max(mnfd, 0);
        maximumFractionDigitsDefault = minimumFractionDigits;
    else // else let mxfdDefault be max(mnfd, 3).
        maximumFractionDigitsDefault = std::max(minimumFractionDigits, 3u);

    // 35. Let mxfd be GetNumberOption(options, "maximumFractionDigits", mnfd, 20, mxfdDefault).
    // 36. ReturnIfAbrupt(mxfd).
    // 37. Set numberFormat.[[maximumFractionDigits]] to mxfd.
    unsigned maximumFractionDigits = intlNumberOption(state, options, Identifier::fromString(&vm, "maximumFractionDigits"), minimumFractionDigits, 20, maximumFractionDigitsDefault);
    if (state.hadException())
        return;
    m_maximumFractionDigits = maximumFractionDigits;

    // 38. Let mnsd be Get(options, "minimumSignificantDigits").
    JSValue minimumSignificantDigitsValue = options->get(&state, Identifier::fromString(&vm, "minimumSignificantDigits"));
    // 39. ReturnIfAbrupt(mnsd).
    if (state.hadException())
        return;

    // 40. Let mxsd be Get(options, "maximumSignificantDigits").
    JSValue maximumSignificantDigitsValue = options->get(&state, Identifier::fromString(&vm, "maximumSignificantDigits"));
    // 41. ReturnIfAbrupt(mxsd).
    if (state.hadException())
        return;

    // 42. If mnsd is not undefined or mxsd is not undefined, then
    if (!minimumSignificantDigitsValue.isUndefined() || !maximumSignificantDigitsValue.isUndefined()) {
        // a. Let mnsd be GetNumberOption(options, "minimumSignificantDigits", 1, 21, 1).
        unsigned minimumSignificantDigits = intlNumberOption(state, options, Identifier::fromString(&vm, "minimumSignificantDigits"), 1, 21, 1);
        // b. ReturnIfAbrupt(mnsd).
        if (state.hadException())
            return;
        // c. Let mxsd be GetNumberOption(options, "maximumSignificantDigits", mnsd, 21, 21).
        unsigned maximumSignificantDigits = intlNumberOption(state, options, Identifier::fromString(&vm, "maximumSignificantDigits"), minimumSignificantDigits, 21, 21);
        // d. ReturnIfAbrupt(mxsd).
        if (state.hadException())
            return;
        // e. Set numberFormat.[[minimumSignificantDigits]] to mnsd.
        m_minimumSignificantDigits = minimumSignificantDigits;
        // f. Set numberFormat.[[maximumSignificantDigits]] to mxsd.
        m_maximumSignificantDigits = maximumSignificantDigits;
    }

    // 43. Let g be GetOption(options, "useGrouping", "boolean", undefined, true).
    bool usesFallback;
    bool useGrouping = intlBooleanOption(state, options, Identifier::fromString(&vm, "useGrouping"), usesFallback);
    if (usesFallback)
        useGrouping = true;
    // 44. ReturnIfAbrupt(g).
    if (state.hadException())
        return;
    // 45. Set numberFormat.[[useGrouping]] to g.
    m_useGrouping = useGrouping;

    // FIXME: Implement Steps 46 - 51.
    // 46. Let dataLocaleData be Get(localeData, dataLocale).
    // 47. Let patterns be Get(dataLocaleData, "patterns").
    // 48. Assert: patterns is an object (see 11.2.3).
    // 49. Let stylePatterns be Get(patterns, s).
    // 50. Set numberFormat.[[positivePattern]] to Get(stylePatterns, "positivePattern").
    // 51. Set numberFormat.[[negativePattern]] to Get(stylePatterns, "negativePattern").

    // 52. Set numberFormat.[[boundFormat]] to undefined.
    // 53. Set numberFormat.[[initializedNumberFormat]] to true.
    m_initializedNumberFormat = true;

    // 54. Return numberFormat.
}

EncodedJSValue JSC_HOST_CALL IntlNumberFormatFuncFormatNumber(ExecState* state)
{
    // 11.3.4 Format Number Functions (ECMA-402 2.0)
    // 1. Let nf be the this value.
    IntlNumberFormat* format = jsDynamicCast<IntlNumberFormat*>(state->thisValue());
    // 2. Assert: Type(nf) is Object and nf has an [[initializedNumberFormat]] internal slot whose value is true.
    if (!format)
        return JSValue::encode(throwTypeError(state));

    // 3. If value is not provided, let value be undefined.
    // 4. Let x be ToNumber(value).
    double value = state->argument(0).toNumber(state);
    // 5. ReturnIfAbrupt(x).
    if (state->hadException())
        return JSValue::encode(jsUndefined());

    // 6. Return FormatNumber(nf, x).
    
    // 11.3.4 FormatNumber abstract operation (ECMA-402 2.0)
    // FIXME: Implement FormatNumber.

    return JSValue::encode(jsNumber(value).toString(state));
}

const char* IntlNumberFormat::styleString(Style style)
{
    switch (style) {
    case Style::Decimal:
        return "decimal";
    case Style::Percent:
        return "percent";
    case Style::Currency:
        return "currency";
    }
    ASSERT_NOT_REACHED();
    return nullptr;
}

const char* IntlNumberFormat::currencyDisplayString(CurrencyDisplay currencyDisplay)
{
    switch (currencyDisplay) {
    case CurrencyDisplay::Code:
        return "code";
    case CurrencyDisplay::Symbol:
        return "symbol";
    case CurrencyDisplay::Name:
        return "name";
    }
    ASSERT_NOT_REACHED();
    return nullptr;
}

JSObject* IntlNumberFormat::resolvedOptions(ExecState& state)
{
    // 11.3.5 Intl.NumberFormat.prototype.resolvedOptions() (ECMA-402 2.0)
    // The function returns a new object whose properties and attributes are set as if
    // constructed by an object literal assigning to each of the following properties the
    // value of the corresponding internal slot of this NumberFormat object (see 11.4):
    // locale, numberingSystem, style, currency, currencyDisplay, minimumIntegerDigits,
    // minimumFractionDigits, maximumFractionDigits, minimumSignificantDigits,
    // maximumSignificantDigits, and useGrouping. Properties whose corresponding internal
    // slots are not present are not assigned.

    if (!m_initializedNumberFormat) {
        initializeNumberFormat(state, jsUndefined(), jsUndefined());
        ASSERT(!state.hadException());
    }

    VM& vm = state.vm();
    JSObject* options = constructEmptyObject(&state);
    options->putDirect(vm, vm.propertyNames->locale, jsString(&state, m_locale));
    options->putDirect(vm, Identifier::fromString(&vm, "numberingSystem"), jsString(&state, m_numberingSystem));
    options->putDirect(vm, Identifier::fromString(&vm, "style"), jsNontrivialString(&state, ASCIILiteral(styleString(m_style))));
    if (m_style == Style::Currency) {
        options->putDirect(vm, Identifier::fromString(&vm, "currency"), jsNontrivialString(&state, m_currency));
        options->putDirect(vm, Identifier::fromString(&vm, "currencyDisplay"), jsNontrivialString(&state, ASCIILiteral(currencyDisplayString(m_currencyDisplay))));
    }
    options->putDirect(vm, Identifier::fromString(&vm, "minimumIntegerDigits"), jsNumber(m_minimumIntegerDigits));
    options->putDirect(vm, Identifier::fromString(&vm, "minimumFractionDigits"), jsNumber(m_minimumFractionDigits));
    options->putDirect(vm, Identifier::fromString(&vm, "maximumFractionDigits"), jsNumber(m_maximumFractionDigits));
    if (m_minimumSignificantDigits) {
        ASSERT(m_maximumSignificantDigits);
        options->putDirect(vm, Identifier::fromString(&vm, "minimumSignificantDigits"), jsNumber(m_minimumSignificantDigits));
        options->putDirect(vm, Identifier::fromString(&vm, "maximumSignificantDigits"), jsNumber(m_maximumSignificantDigits));
    }
    options->putDirect(vm, Identifier::fromString(&vm, "useGrouping"), jsBoolean(m_useGrouping));
    return options;
}

void IntlNumberFormat::setBoundFormat(VM& vm, JSBoundFunction* format)
{
    m_boundFormat.set(vm, this, format);
}

} // namespace JSC

#endif // ENABLE(INTL)
