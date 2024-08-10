# Copyright (C) 2012-2017 Apple Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""
    LLDB Support for WebKit Types

    Add the following to your .lldbinit file to add WebKit Type summaries in LLDB and Xcode:

    command script import {Path to WebKit Root}/Tools/lldb/lldb_webkit.py

"""

import lldb
import string
import struct

def __lldb_init_module(debugger, dict):
    debugger.HandleCommand('command script add -f lldb_webkit.btjs btjs')
    debugger.HandleCommand('type summary add --expand -F lldb_webkit.WTFString_SummaryProvider WTF::String')
    debugger.HandleCommand('type summary add --expand -F lldb_webkit.WTFStringImpl_SummaryProvider WTF::StringImpl')
    debugger.HandleCommand('type summary add --expand -F lldb_webkit.WTFStringView_SummaryProvider WTF::StringView')
    debugger.HandleCommand('type summary add --expand -F lldb_webkit.WTFAtomicString_SummaryProvider WTF::AtomicString')
    debugger.HandleCommand('type summary add --expand -F lldb_webkit.WTFVector_SummaryProvider -x "^WTF::Vector<.+>$"')
    debugger.HandleCommand('type summary add --expand -F lldb_webkit.WTFHashTable_SummaryProvider -x "^WTF::HashTable<.+>$"')
    debugger.HandleCommand('type summary add --expand -F lldb_webkit.WTFHashMap_SummaryProvider -x "^WTF::HashMap<.+>$"')
    debugger.HandleCommand('type summary add --expand -F lldb_webkit.WTFHashSet_SummaryProvider -x "^WTF::HashSet<.+>$"')
    debugger.HandleCommand('type summary add --expand -F lldb_webkit.WTFMediaTime_SummaryProvider WTF::MediaTime')

    debugger.HandleCommand('type summary add -F lldb_webkit.WebCoreURL_SummaryProvider WebCore::URL')
    debugger.HandleCommand('type summary add -F lldb_webkit.WebCoreColor_SummaryProvider WebCore::Color')

    debugger.HandleCommand('type summary add -F lldb_webkit.WebCoreLayoutUnit_SummaryProvider WebCore::LayoutUnit')
    debugger.HandleCommand('type summary add -F lldb_webkit.WebCoreLayoutSize_SummaryProvider WebCore::LayoutSize')
    debugger.HandleCommand('type summary add -F lldb_webkit.WebCoreLayoutPoint_SummaryProvider WebCore::LayoutPoint')
    debugger.HandleCommand('type summary add -F lldb_webkit.WebCoreLayoutRect_SummaryProvider WebCore::LayoutRect')

    debugger.HandleCommand('type summary add -F lldb_webkit.WebCoreIntSize_SummaryProvider WebCore::IntSize')
    debugger.HandleCommand('type summary add -F lldb_webkit.WebCoreIntPoint_SummaryProvider WebCore::IntPoint')
    debugger.HandleCommand('type summary add -F lldb_webkit.WebCoreIntRect_SummaryProvider WebCore::IntRect')

    debugger.HandleCommand('type summary add -F lldb_webkit.WebCoreFloatSize_SummaryProvider WebCore::FloatSize')
    debugger.HandleCommand('type summary add -F lldb_webkit.WebCoreFloatPoint_SummaryProvider WebCore::FloatPoint')
    debugger.HandleCommand('type summary add -F lldb_webkit.WebCoreFloatRect_SummaryProvider WebCore::FloatRect')

    # synthetic types (see <https://lldb.llvm.org/varformats.html>)
    debugger.HandleCommand('type synthetic add -x "^WTF::Vector<.+>$" --python-class lldb_webkit.WTFVectorProvider')
    debugger.HandleCommand('type synthetic add -x "^WTF::HashTable<.+>$" --python-class lldb_webkit.WTFHashTableProvider')


def WTFString_SummaryProvider(valobj, dict):
    provider = WTFStringProvider(valobj, dict)
    return "{ length = %d, contents = '%s' }" % (provider.get_length(), provider.to_string())


def WTFStringImpl_SummaryProvider(valobj, dict):
    provider = WTFStringImplProvider(valobj, dict)
    if not provider.is_initialized():
        return ""
    return "{ length = %d, is8bit = %d, contents = '%s' }" % (provider.get_length(), provider.is_8bit(), provider.to_string())


def WTFStringView_SummaryProvider(valobj, dict):
    provider = WTFStringViewProvider(valobj, dict)
    return "{ length = %d, contents = '%s' }" % (provider.get_length(), provider.to_string())


def WTFAtomicString_SummaryProvider(valobj, dict):
    return WTFString_SummaryProvider(valobj.GetChildMemberWithName('m_string'), dict)


def WTFVector_SummaryProvider(valobj, dict):
    provider = WTFVectorProvider(valobj, dict)
    return "{ size = %d, capacity = %d }" % (provider.size, provider.capacity)


def WTFHashTable_SummaryProvider(valobj, dict):
    provider = WTFHashTableProvider(valobj, dict)
    return "{ tableSize = %d, keyCount = %d }" % (provider.tableSize(), provider.keyCount())


def WTFHashMap_SummaryProvider(valobj, dict):
    provider = WTFHashMapProvider(valobj, dict)
    return "{ tableSize = %d, keyCount = %d }" % (provider.tableSize(), provider.keyCount())


def WTFHashSet_SummaryProvider(valobj, dict):
    provider = WTFHashSetProvider(valobj, dict)
    return "{ tableSize = %d, keyCount = %d }" % (provider.tableSize(), provider.keyCount())


def WTFMediaTime_SummaryProvider(valobj, dict):
    provider = WTFMediaTimeProvider(valobj, dict)
    if provider.isInvalid():
        return "{ Invalid }"
    if provider.isPositiveInfinity():
        return "{ +Infinity }"
    if provider.isNegativeInfinity():
        return "{ -Infinity }"
    if provider.isIndefinite():
        return "{ Indefinite }"
    if provider.hasDoubleValue():
        return "{ %f }" % (provider.timeValueAsDouble())
    return "{ %d/%d, %f }" % (provider.timeValue(), provider.timeScale(), float(provider.timeValue()) / provider.timeScale())


def WebCoreColor_SummaryProvider(valobj, dict):
    provider = WebCoreColorProvider(valobj, dict)
    return "{ %s }" % provider.to_string()


def WebCoreURL_SummaryProvider(valobj, dict):
    provider = WebCoreURLProvider(valobj, dict)
    return "{ %s }" % provider.to_string()


def WebCoreLayoutUnit_SummaryProvider(valobj, dict):
    provider = WebCoreLayoutUnitProvider(valobj, dict)
    return "{ %s }" % provider.to_string()


def WebCoreLayoutSize_SummaryProvider(valobj, dict):
    provider = WebCoreLayoutSizeProvider(valobj, dict)
    return "{ width = %s, height = %s }" % (provider.get_width(), provider.get_height())


def WebCoreLayoutPoint_SummaryProvider(valobj, dict):
    provider = WebCoreLayoutPointProvider(valobj, dict)
    return "{ x = %s, y = %s }" % (provider.get_x(), provider.get_y())


def WebCoreLayoutRect_SummaryProvider(valobj, dict):
    provider = WebCoreLayoutRectProvider(valobj, dict)
    return "{ x = %s, y = %s, width = %s, height = %s }" % (provider.get_x(), provider.get_y(), provider.get_width(), provider.get_height())


def WebCoreIntSize_SummaryProvider(valobj, dict):
    provider = WebCoreIntSizeProvider(valobj, dict)
    return "{ width = %s, height = %s }" % (provider.get_width(), provider.get_height())


def WebCoreIntPoint_SummaryProvider(valobj, dict):
    provider = WebCoreIntPointProvider(valobj, dict)
    return "{ x = %s, y = %s }" % (provider.get_x(), provider.get_y())


def WebCoreFloatSize_SummaryProvider(valobj, dict):
    provider = WebCoreFloatSizeProvider(valobj, dict)
    return "{ width = %s, height = %s }" % (provider.get_width(), provider.get_height())


def WebCoreFloatPoint_SummaryProvider(valobj, dict):
    provider = WebCoreFloatPointProvider(valobj, dict)
    return "{ x = %s, y = %s }" % (provider.get_x(), provider.get_y())


def WebCoreIntRect_SummaryProvider(valobj, dict):
    provider = WebCoreIntRectProvider(valobj, dict)
    return "{ x = %s, y = %s, width = %s, height = %s }" % (provider.get_x(), provider.get_y(), provider.get_width(), provider.get_height())


def WebCoreFloatRect_SummaryProvider(valobj, dict):
    provider = WebCoreFloatRectProvider(valobj, dict)
    return "{ x = %s, y = %s, width = %s, height = %s }" % (provider.get_x(), provider.get_y(), provider.get_width(), provider.get_height())




def btjs(debugger, command, result, internal_dict):
    '''Prints a stack trace of current thread with JavaScript frames decoded.  Takes optional frame count argument'''

    target = debugger.GetSelectedTarget()
    addressFormat = '#0{width}x'.format(width=target.GetAddressByteSize() * 2 + 2)
    process = target.GetProcess()
    thread = process.GetSelectedThread()

    if target.FindFunctions("JSC::ExecState::describeFrame").GetSize() or target.FindFunctions("_ZN3JSC9ExecState13describeFrameEv").GetSize():
        annotateJSFrames = True
    else:
        annotateJSFrames = False

    if not annotateJSFrames:
        print("Warning: Can't find JSC::ExecState::describeFrame() in executable to annotate JavaScript frames")

    backtraceDepth = thread.GetNumFrames()

    if len(command) > 0:
        try:
            backtraceDepth = int(command)
        except ValueError:
            return

    threadFormat = '* thread #{num}: tid = {tid:#x}, {pcAddr:' + addressFormat + '}, queue = \'{queueName}, stop reason = {stopReason}'
    print(threadFormat.format(num=thread.GetIndexID(), tid=thread.GetThreadID(), pcAddr=thread.GetFrameAtIndex(0).GetPC(), queueName=thread.GetQueueName(), stopReason=thread.GetStopDescription(30)))

    for frame in thread:
        if backtraceDepth < 1:
            break

        backtraceDepth = backtraceDepth - 1

        function = frame.GetFunction()

        if annotateJSFrames and not frame or not frame.GetSymbol() or frame.GetSymbol().GetName() == "llint_entry":
            callFrame = frame.GetSP()
            JSFrameDescription = frame.EvaluateExpression("((JSC::ExecState*)0x%x)->describeFrame()" % frame.GetFP()).GetSummary()
            if not JSFrameDescription:
                JSFrameDescription = frame.EvaluateExpression("((JSC::CallFrame*)0x%x)->describeFrame()" % frame.GetFP()).GetSummary()
            if not JSFrameDescription:
                JSFrameDescription = frame.EvaluateExpression("(char*)_ZN3JSC9ExecState13describeFrameEv(0x%x)" % frame.GetFP()).GetSummary()
            if JSFrameDescription:
                JSFrameDescription = string.strip(JSFrameDescription, '"')
                frameFormat = '    frame #{num}: {addr:' + addressFormat + '} {desc}'
                print(frameFormat.format(num=frame.GetFrameID(), addr=frame.GetPC(), desc=JSFrameDescription))
                continue
        print('    %s' % frame)

# FIXME: Provide support for the following types:
# def WTFVector_SummaryProvider(valobj, dict):
# def WTFCString_SummaryProvider(valobj, dict):
# def WebCoreQualifiedName_SummaryProvider(valobj, dict):
# def JSCIdentifier_SummaryProvider(valobj, dict):
# def JSCJSString_SummaryProvider(valobj, dict):


def guess_string_length(valobj, charSize, error):
    if not valobj.GetValue():
        return 0

    maxLength = 256

    pointer = valobj.GetValueAsUnsigned()
    contents = valobj.GetProcess().ReadMemory(pointer, maxLength * charSize, lldb.SBError())
    format = 'B' if charSize == 1 else 'H'

    for i in xrange(0, maxLength):
        if not struct.unpack_from(format, contents, i * charSize)[0]:
            return i

    return maxLength

def ustring_to_string(valobj, error, length=None):
    if length is None:
        length = guess_string_length(valobj, 2, error)
    else:
        length = int(length)

    if length == 0:
        return ""

    pointer = valobj.GetValueAsUnsigned()
    contents = valobj.GetProcess().ReadMemory(pointer, length * 2, lldb.SBError())

    # lldb does not (currently) support returning unicode from python summary providers,
    # so potentially convert this to ascii by escaping
    string = contents.decode('utf16')
    try:
        return str(string)
    except:
        return string.encode('unicode_escape')

def lstring_to_string(valobj, error, length=None):
    if length is None:
        length = guess_string_length(valobj, 1, error)
    else:
        length = int(length)

    if length == 0:
        return ""

    pointer = valobj.GetValueAsUnsigned()
    contents = valobj.GetProcess().ReadMemory(pointer, length, lldb.SBError())

    # lldb does not (currently) support returning unicode from python summary providers,
    # so potentially convert this to ascii by escaping
    string = contents.decode('utf8')
    try:
        return str(string)
    except:
        return string.encode('unicode_escape')

class WTFStringImplProvider:
    def __init__(self, valobj, dict):
        # FIXME: For some reason lldb(1) sometimes has an issue accessing members of WTF::StringImplShape
        # via a WTF::StringImpl pointer (why?). As a workaround we explicitly cast to WTF::StringImplShape*.
        string_impl_shape_ptr_type = valobj.GetTarget().FindFirstType('WTF::StringImplShape').GetPointerType()
        self.valobj = valobj.Cast(string_impl_shape_ptr_type)

    def get_length(self):
        return self.valobj.GetChildMemberWithName('m_length').GetValueAsUnsigned(0)

    def get_data8(self):
        return self.valobj.GetChildAtIndex(2).GetChildMemberWithName('m_data8')

    def get_data16(self):
        return self.valobj.GetChildAtIndex(2).GetChildMemberWithName('m_data16')

    def to_string(self):
        error = lldb.SBError()

        if not self.is_initialized():
            return u""

        if self.is_8bit():
            return lstring_to_string(self.get_data8(), error, self.get_length())
        return ustring_to_string(self.get_data16(), error, self.get_length())

    def is_8bit(self):
        # FIXME: find a way to access WTF::StringImpl::s_hashFlag8BitBuffer
        return bool(self.valobj.GetChildMemberWithName('m_hashAndFlags').GetValueAsUnsigned(0) \
            & 1 << 3)

    def is_initialized(self):
        return self.valobj.GetValueAsUnsigned() != 0


class WTFStringViewProvider:
    def __init__(self, valobj, dict):
        self.valobj = valobj

    def is_8bit(self):
        return bool(self.valobj.GetChildMemberWithName('m_is8Bit').GetValueAsUnsigned(0))

    def get_length(self):
        return self.valobj.GetChildMemberWithName('m_length').GetValueAsUnsigned(0)

    def get_characters(self):
        return self.valobj.GetChildMemberWithName('m_characters')

    def to_string(self):
        error = lldb.SBError()

        if not self.get_characters() or not self.get_length():
            return u""

        if self.is_8bit():
            return lstring_to_string(self.get_characters(), error, self.get_length())
        return ustring_to_string(self.get_characters(), error, self.get_length())


class WTFStringProvider:
    def __init__(self, valobj, dict):
        self.valobj = valobj

    def stringimpl(self):
        impl_ptr = self.valobj.GetChildMemberWithName('m_impl').GetChildMemberWithName('m_ptr')
        return WTFStringImplProvider(impl_ptr, dict)

    def get_length(self):
        impl = self.stringimpl()
        if not impl:
            return 0
        return impl.get_length()

    def to_string(self):
        impl = self.stringimpl()
        if not impl:
            return u""
        return impl.to_string()


class WebCoreColorProvider:
    "Print a WebCore::Color"
    def __init__(self, valobj, dict):
        self.valobj = valobj

    def _is_extended(self, rgba_and_flags):
        return not bool(rgba_and_flags & 0x1)

    def _is_valid(self, rgba_and_flags):
        # Assumes not extended.
        return bool(rgba_and_flags & 0x2)

    def _is_semantic(self, rgba_and_flags):
        # Assumes not extended.
        return bool(rgba_and_flags & 0x4)

    def _to_string_extended(self):
        extended_color = self.valobj.GetChildMemberWithName('m_colorData').GetChildMemberWithName('extendedColor').Dereference()
        profile = extended_color.GetChildMemberWithName('m_colorSpace').GetValue()
        if profile == 'ColorSpaceSRGB':
            profile = 'srgb'
        elif profile == 'ColorSpaceDisplayP3':
            profile = 'display-p3'
        else:
            profile = 'unknown'
        red = float(extended_color.GetChildMemberWithName('m_red').GetValue())
        green = float(extended_color.GetChildMemberWithName('m_green').GetValue())
        blue = float(extended_color.GetChildMemberWithName('m_blue').GetValue())
        alpha = float(extended_color.GetChildMemberWithName('m_alpha').GetValue())
        return "color(%s %1.2f %1.2f %1.2f / %1.2f)" % (profile, red, green, blue, alpha)

    def to_string(self):
        rgba_and_flags = self.valobj.GetChildMemberWithName('m_colorData').GetChildMemberWithName('rgbaAndFlags').GetValueAsUnsigned(0)

        if self._is_extended(rgba_and_flags):
            return self._to_string_extended()

        if not self._is_valid(rgba_and_flags):
            return 'invalid'

        color = rgba_and_flags >> 32
        red = (color >> 16) & 0xFF
        green = (color >> 8) & 0xFF
        blue = color & 0xFF
        alpha = ((color >> 24) & 0xFF) / 255.0

        semantic = ' semantic' if self._is_semantic(rgba_and_flags) else ""

        result = 'rgba(%d, %d, %d, %1.2f)%s' % (red, green, blue, alpha, semantic)
        return result


class WebCoreLayoutUnitProvider:
    "Print a WebCore::LayoutUnit"
    def __init__(self, valobj, dict):
        self.valobj = valobj

    def to_string(self):
        layoutUnitValue = self.valobj.GetChildMemberWithName('m_value').GetValueAsSigned(0)
        return "%gpx (%d)" % (float(layoutUnitValue) / 64, layoutUnitValue)


class WebCoreLayoutSizeProvider:
    "Print a WebCore::LayoutSize"
    def __init__(self, valobj, dict):
        self.valobj = valobj

    def get_width(self):
        return WebCoreLayoutUnitProvider(self.valobj.GetChildMemberWithName('m_width'), dict).to_string()

    def get_height(self):
        return WebCoreLayoutUnitProvider(self.valobj.GetChildMemberWithName('m_height'), dict).to_string()


class WebCoreLayoutPointProvider:
    "Print a WebCore::LayoutPoint"
    def __init__(self, valobj, dict):
        self.valobj = valobj

    def get_x(self):
        return WebCoreLayoutUnitProvider(self.valobj.GetChildMemberWithName('m_x'), dict).to_string()

    def get_y(self):
        return WebCoreLayoutUnitProvider(self.valobj.GetChildMemberWithName('m_y'), dict).to_string()


class WebCoreLayoutRectProvider:
    "Print a WebCore::LayoutRect"
    def __init__(self, valobj, dict):
        self.valobj = valobj

    def get_x(self):
        return WebCoreLayoutPointProvider(self.valobj.GetChildMemberWithName('m_location'), dict).get_x()

    def get_y(self):
        return WebCoreLayoutPointProvider(self.valobj.GetChildMemberWithName('m_location'), dict).get_y()

    def get_width(self):
        return WebCoreLayoutSizeProvider(self.valobj.GetChildMemberWithName('m_size'), dict).get_width()

    def get_height(self):
        return WebCoreLayoutSizeProvider(self.valobj.GetChildMemberWithName('m_size'), dict).get_height()


class WebCoreIntPointProvider:
    "Print a WebCore::IntPoint"
    def __init__(self, valobj, dict):
        self.valobj = valobj

    def get_x(self):
        return self.valobj.GetChildMemberWithName('m_x').GetValueAsSigned()

    def get_y(self):
        return self.valobj.GetChildMemberWithName('m_y').GetValueAsSigned()


class WebCoreIntSizeProvider:
    "Print a WebCore::IntSize"
    def __init__(self, valobj, dict):
        self.valobj = valobj

    def get_width(self):
        return self.valobj.GetChildMemberWithName('m_width').GetValueAsSigned()

    def get_height(self):
        return self.valobj.GetChildMemberWithName('m_height').GetValueAsSigned()


class WebCoreIntRectProvider:
    "Print a WebCore::IntRect"
    def __init__(self, valobj, dict):
        self.valobj = valobj

    def get_x(self):
        return WebCoreIntPointProvider(self.valobj.GetChildMemberWithName('m_location'), dict).get_x()

    def get_y(self):
        return WebCoreIntPointProvider(self.valobj.GetChildMemberWithName('m_location'), dict).get_y()

    def get_width(self):
        return WebCoreIntSizeProvider(self.valobj.GetChildMemberWithName('m_size'), dict).get_width()

    def get_height(self):
        return WebCoreIntSizeProvider(self.valobj.GetChildMemberWithName('m_size'), dict).get_height()


class WebCoreFloatPointProvider:
    "Print a WebCore::FloatPoint"
    def __init__(self, valobj, dict):
        self.valobj = valobj

    def get_x(self):
        return float(self.valobj.GetChildMemberWithName('m_x').GetValue())

    def get_y(self):
        return float(self.valobj.GetChildMemberWithName('m_y').GetValue())


class WebCoreFloatSizeProvider:
    "Print a WebCore::FloatSize"
    def __init__(self, valobj, dict):
        self.valobj = valobj

    def get_width(self):
        return float(self.valobj.GetChildMemberWithName('m_width').GetValue())

    def get_height(self):
        return float(self.valobj.GetChildMemberWithName('m_height').GetValue())


class WebCoreFloatRectProvider:
    "Print a WebCore::FloatRect"
    def __init__(self, valobj, dict):
        self.valobj = valobj

    def get_x(self):
        return WebCoreFloatPointProvider(self.valobj.GetChildMemberWithName('m_location'), dict).get_x()

    def get_y(self):
        return WebCoreFloatPointProvider(self.valobj.GetChildMemberWithName('m_location'), dict).get_y()

    def get_width(self):
        return WebCoreFloatSizeProvider(self.valobj.GetChildMemberWithName('m_size'), dict).get_width()

    def get_height(self):
        return WebCoreFloatSizeProvider(self.valobj.GetChildMemberWithName('m_size'), dict).get_height()


class WebCoreURLProvider:
    "Print a WebCore::URL"
    def __init__(self, valobj, dict):
        self.valobj = valobj

    def to_string(self):
        return WTFStringProvider(self.valobj.GetChildMemberWithName('m_string'), dict).to_string()


class WTFVectorProvider:
    def __init__(self, valobj, internal_dict):
        self.valobj = valobj
        self.update()

    def num_children(self):
        return self.size + 3

    def get_child_index(self, name):
        if name == "m_size":
            return self.size
        elif name == "m_capacity":
            return self.size + 1
        elif name == "m_buffer":
            return self.size + 2
        else:
            return int(name.lstrip('[').rstrip(']'))

    def get_child_at_index(self, index):
        if index == self.size:
            return self.valobj.GetChildMemberWithName("m_size")
        elif index == self.size + 1:
            return self.valobj.GetChildMemberWithName("m_capacity")
        elif index == self.size + 2:
            return self.buffer
        elif index < self.size:
            offset = index * self.data_size
            child = self.buffer.CreateChildAtOffset('[' + str(index) + ']', offset, self.data_type)
            return child
        else:
            return None

    def update(self):
        self.buffer = self.valobj.GetChildMemberWithName('m_buffer')
        self.size = self.valobj.GetChildMemberWithName('m_size').GetValueAsUnsigned(0)
        self.capacity = self.valobj.GetChildMemberWithName('m_capacity').GetValueAsUnsigned(0)
        self.data_type = self.buffer.GetType().GetPointeeType()
        self.data_size = self.data_type.GetByteSize()

    def has_children(self):
        return True


class WTFHashMapProvider:
    def __init__(self, valobj, internal_dict):
        self.valobj = valobj
        impl_ptr = self.valobj.GetChildMemberWithName('m_impl')
        self._hash_table_provider = WTFHashTableProvider(impl_ptr, dict)

    def tableSize(self):
        return self._hash_table_provider.tableSize()

    def keyCount(self):
        return self._hash_table_provider.keyCount()


class WTFHashSetProvider:
    def __init__(self, valobj, internal_dict):
        self.valobj = valobj
        impl_ptr = self.valobj.GetChildMemberWithName('m_impl')
        self._hash_table_provider = WTFHashTableProvider(impl_ptr, dict)

    def tableSize(self):
        return self._hash_table_provider.tableSize()

    def keyCount(self):
        return self._hash_table_provider.keyCount()


class WTFHashTableProvider:
    def __init__(self, valobj, internal_dict):
        self.valobj = valobj
        self.update()

    def tableSize(self):
        return self.valobj.GetChildMemberWithName('m_tableSize').GetValueAsUnsigned(0)

    def keyCount(self):
        return self.valobj.GetChildMemberWithName('m_keyCount').GetValueAsUnsigned(0)

    # Synthetic children provider methods.
    def num_children(self):
        return self.tableSize() + 5

    def get_child_index(self, name):
        if name == "m_table":
            return self.tableSize()
        elif name == "m_tableSize":
            return self.tableSize() + 1
        elif name == "m_tableSizeMask":
            return self.tableSize() + 2
        elif name == "m_keyCount":
            return self.tableSize() + 3
        elif name == "m_deletedCount":
            return self.tableSize() + 4
        else:
            return int(name.lstrip('[').rstrip(']'))

    def get_child_at_index(self, index):
        if index == self.tableSize():
            return self.valobj.GetChildMemberWithName('m_table')
        elif index == self.tableSize() + 1:
            return self.valobj.GetChildMemberWithName('m_tableSize')
        elif index == self.tableSize() + 2:
            return self.valobj.GetChildMemberWithName('m_tableSizeMask')
        elif index == self.tableSize() + 3:
            return self.valobj.GetChildMemberWithName('m_keyCount')
        elif index == self.tableSize() + 4:
            return self.valobj.GetChildMemberWithName('m_deletedCount')
        elif index < self.tableSize():
            table = self.valobj.GetChildMemberWithName('m_table')
            return table.CreateChildAtOffset('[' + str(index) + ']', index * self.data_size, self.data_type)
        else:
            return None

    def update(self):
        self.data_type = self.valobj.GetType().GetTemplateArgumentType(1)
        self.data_size = self.data_type.GetByteSize()

    def has_children(self):
        return True


class WTFMediaTimeProvider:
    def __init__(self, valobj, internal_dict):
        self.valobj = valobj

    def timeValue(self):
        return self.valobj.GetChildMemberWithName('m_timeValue').GetValueAsSigned(0)

    def timeValueAsDouble(self):
        error = lldb.SBError()
        return self.valobj.GetChildMemberWithName('m_timeValueAsDouble').GetData().GetDouble(error, 0)

    def timeScale(self):
        return self.valobj.GetChildMemberWithName('m_timeScale').GetValueAsSigned(0)

    def isInvalid(self):
        return not self.valobj.GetChildMemberWithName('m_timeFlags').GetValueAsSigned(0) & (1 << 0)

    def isPositiveInfinity(self):
        return self.valobj.GetChildMemberWithName('m_timeFlags').GetValueAsSigned(0) & (1 << 2)

    def isNegativeInfinity(self):
        return self.valobj.GetChildMemberWithName('m_timeFlags').GetValueAsSigned(0) & (1 << 3)

    def isIndefinite(self):
        return self.valobj.GetChildMemberWithName('m_timeFlags').GetValueAsSigned(0) & (1 << 4)

    def hasDoubleValue(self):
        return self.valobj.GetChildMemberWithName('m_timeFlags').GetValueAsSigned(0) & (1 << 5)
