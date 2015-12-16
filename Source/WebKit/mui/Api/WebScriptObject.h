/*
 * Copyright (C) 2008 Pleyo.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Pleyo nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PLEYO AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL PLEYO OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebScriptObject_h
#define WebScriptObject_h

class WebScriptObject {
public:

    /**
     * WebScriptObject default constructor
     */
    WebScriptObject();

    /**
     * WebScriptObject destructor
     */
    virtual ~WebScriptObject();


    /**
     * @brief Throws an exception in the current script execution context.
     * @result true if the exception was raised, false if an error occurred.
     */
    virtual bool throwException(const char* exceptionMessage);

    /**
     * @brief remove a web script key 
     * @param name The name of the method to call in the script environment.
     */
    virtual void removeWebScriptKey(const char* name);

    /**
     * @brief Converts the target object to a string representation.  The coercion
     * of non string objects type is dependent on the script environment.
     * @result Returns the string representation of the object.
     */
    virtual const char* stringRepresentation();

    /**
     * @brief Raises an exception in the script environment in the context of the
     * current object.
     * @param description The description of the exception.
     */
    virtual void setException(const char* description);
};

#endif // WebScriptObject_h
