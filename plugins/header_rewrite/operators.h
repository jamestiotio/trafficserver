/*
  Licensed to the Apache Software Foundation (ASF) under one
  or more contributor license agreements.  See the NOTICE file
  distributed with this work for additional information
  regarding copyright ownership.  The ASF licenses this file
  to you under the Apache License, Version 2.0 (the
  "License"); you may not use this file except in compliance
  with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/
//////////////////////////////////////////////////////////////////////////////////////////////
//
// Implement the classes for the various types of hash keys we support.
//
#pragma once

#include <string>

#include "ts/ts.h"

#include "operator.h"
#include "resources.h"
#include "value.h"

///////////////////////////////////////////////////////////////////////////////
// Operator declarations.
//
class OperatorSetConfig : public Operator
{
public:
  OperatorSetConfig() { Dbg(dbg_ctl, "Calling CTOR for OperatorSetConfig"); }

  // noncopyable
  OperatorSetConfig(const OperatorSetConfig &) = delete;
  void operator=(const OperatorSetConfig &)    = delete;

  void initialize(Parser &p) override;

protected:
  void exec(const Resources &res) const override;

private:
  TSOverridableConfigKey _key  = TS_CONFIG_NULL;
  TSRecordDataType       _type = TS_RECORDDATATYPE_NULL;

  std::string _config;
  Value       _value;
};

class OperatorSetStatus : public Operator
{
public:
  OperatorSetStatus() { Dbg(dbg_ctl, "Calling CTOR for OperatorSetStatus"); }

  // noncopyable
  OperatorSetStatus(const OperatorSetStatus &) = delete;
  void operator=(const OperatorSetStatus &)    = delete;

  void initialize(Parser &p) override;

protected:
  void initialize_hooks() override;
  void exec(const Resources &res) const override;

private:
  Value       _status;
  const char *_reason     = nullptr;
  int         _reason_len = 0;
};

class OperatorSetStatusReason : public Operator
{
public:
  OperatorSetStatusReason() { Dbg(dbg_ctl, "Calling CTOR for OperatorSetStatusReason"); }

  // noncopyable
  OperatorSetStatusReason(const OperatorSetStatusReason &) = delete;
  void operator=(const OperatorSetStatusReason &)          = delete;

  void initialize(Parser &p) override;

protected:
  void initialize_hooks() override;
  void exec(const Resources &res) const override;

private:
  Value _reason;
};

class OperatorSetDestination : public Operator
{
public:
  OperatorSetDestination() { Dbg(dbg_ctl, "Calling CTOR for OperatorSetDestination"); }

  // noncopyable
  OperatorSetDestination(const OperatorSetDestination &) = delete;
  void operator=(const OperatorSetDestination &)         = delete;

  void initialize(Parser &p) override;

protected:
  void exec(const Resources &res) const override;

private:
  UrlQualifiers _url_qual = URL_QUAL_NONE;
  Value         _value;
};

// All the header operators share a base class
class OperatorRMDestination : public Operator
{
public:
  OperatorRMDestination() { Dbg(dbg_ctl, "Calling CTOR for OperatorRMDestination"); }

  // noncopyable
  OperatorRMDestination(const OperatorRMDestination &) = delete;
  void operator=(const OperatorRMDestination &)        = delete;

  void initialize(Parser &p) override;

protected:
  void exec(const Resources &res) const override;

private:
  UrlQualifiers _url_qual = URL_QUAL_NONE;
};

class OperatorSetRedirect : public Operator
{
public:
  OperatorSetRedirect() { Dbg(dbg_ctl, "Calling CTOR for OperatorSetRedirect"); }

  // noncopyable
  OperatorSetRedirect(const OperatorSetRedirect &) = delete;
  void operator=(const OperatorSetRedirect &)      = delete;

  void initialize(Parser &p) override;

  TSHttpStatus
  get_status() const
  {
    return static_cast<TSHttpStatus>(_status.get_int_value());
  }

  const std::string &
  get_location() const
  {
    return _location.get_value();
  }

protected:
  void exec(const Resources &res) const override;

private:
  Value _status;
  Value _location;
};

class OperatorNoOp : public Operator
{
public:
  OperatorNoOp() { Dbg(dbg_ctl, "Calling CTOR for OperatorNoOp"); }

  // noncopyable
  OperatorNoOp(const OperatorNoOp &)   = delete;
  void operator=(const OperatorNoOp &) = delete;

protected:
  void exec(const Resources & /* res ATS_UNUSED */) const override {};
};

class OperatorSetTimeoutOut : public Operator
{
public:
  OperatorSetTimeoutOut() { Dbg(dbg_ctl, "Calling CTOR for OperatorSetTimeoutOut"); }

  // noncopyable
  OperatorSetTimeoutOut(const OperatorSetTimeoutOut &) = delete;
  void operator=(const OperatorSetTimeoutOut &)        = delete;

  void initialize(Parser &p) override;

protected:
  void exec(const Resources &res) const override;

private:
  enum TimeoutOutType {
    TO_OUT_UNDEFINED,
    TO_OUT_ACTIVE,
    TO_OUT_INACTIVE,
    TO_OUT_CONNECT,
    TO_OUT_DNS,
  };

  TimeoutOutType _type = TO_OUT_UNDEFINED;
  Value          _timeout;
};

class OperatorSkipRemap : public Operator
{
public:
  OperatorSkipRemap() { Dbg(dbg_ctl, "Calling CTOR for OperatorSkipRemap"); }

  // noncopyable
  OperatorSkipRemap(const OperatorSkipRemap &) = delete;
  void operator=(const OperatorSkipRemap &)    = delete;

  void initialize(Parser &p) override;

protected:
  void exec(const Resources &res) const override;

private:
  bool _skip_remap = false;
};

// All the header operators share a base class
class OperatorRMHeader : public OperatorHeaders
{
public:
  OperatorRMHeader() { Dbg(dbg_ctl, "Calling CTOR for OperatorRMHeader"); }

  // noncopyable
  OperatorRMHeader(const OperatorRMHeader &) = delete;
  void operator=(const OperatorRMHeader &)   = delete;

protected:
  void exec(const Resources &res) const override;
};

class OperatorAddHeader : public OperatorHeaders
{
public:
  OperatorAddHeader() { Dbg(dbg_ctl, "Calling CTOR for OperatorAddHeader"); }

  // noncopyable
  OperatorAddHeader(const OperatorAddHeader &) = delete;
  void operator=(const OperatorAddHeader &)    = delete;

  void initialize(Parser &p) override;

protected:
  void exec(const Resources &res) const override;

private:
  Value _value;
};

class OperatorSetHeader : public OperatorHeaders
{
public:
  OperatorSetHeader() { Dbg(dbg_ctl, "Calling CTOR for OperatorSetHeader"); }

  // noncopyable
  OperatorSetHeader(const OperatorSetHeader &) = delete;
  void operator=(const OperatorSetHeader &)    = delete;

  void initialize(Parser &p) override;

protected:
  void exec(const Resources &res) const override;

private:
  Value _value;
};

class OperatorCounter : public Operator
{
public:
  OperatorCounter() { Dbg(dbg_ctl, "Calling CTOR for OperatorCounter"); }

  // noncopyable
  OperatorCounter(const OperatorCounter &) = delete;
  void operator=(const OperatorCounter &)  = delete;

  void initialize(Parser &p) override;

protected:
  void exec(const Resources &res) const override;

private:
  std::string _counter_name;
  int         _counter = TS_ERROR;
};

class OperatorRMCookie : public OperatorCookies
{
public:
  OperatorRMCookie() { Dbg(dbg_ctl, "Calling CTOR for OperatorRMCookie"); }

  // noncopyable
  OperatorRMCookie(const OperatorRMCookie &) = delete;
  void operator=(const OperatorRMCookie &)   = delete;

protected:
  void exec(const Resources &res) const override;
};

class OperatorAddCookie : public OperatorCookies
{
public:
  OperatorAddCookie() { Dbg(dbg_ctl, "Calling CTOR for OperatorAddCookie"); }

  // noncopyable
  OperatorAddCookie(const OperatorAddCookie &) = delete;
  void operator=(const OperatorAddCookie &)    = delete;

  void initialize(Parser &p) override;

protected:
  void exec(const Resources &res) const override;

private:
  Value _value;
};

class OperatorSetCookie : public OperatorCookies
{
public:
  OperatorSetCookie() { Dbg(dbg_ctl, "Calling CTOR for OperatorSetCookie"); }

  // noncopyable
  OperatorSetCookie(const OperatorSetCookie &) = delete;
  void operator=(const OperatorSetCookie &)    = delete;

  void initialize(Parser &p) override;

protected:
  void exec(const Resources &res) const override;

private:
  Value _value;
};

namespace CookieHelper
{
enum CookieOp { COOKIE_OP_DEL, COOKIE_OP_ADD, COOKIE_OP_SET };

/*
 * This function returns if cookies need to be changed or not.
 * If the return value is true, updated_cookies would be cookies after the change.
 */
bool cookieModifyHelper(const char *cookies, const size_t cookies_len, std::string &updated_cookies, const CookieOp cookie_op,
                        const std::string &cookie_key, const std::string &cookie_value = std::string());
} // namespace CookieHelper

class OperatorSetConnDSCP : public Operator
{
public:
  OperatorSetConnDSCP() { Dbg(dbg_ctl, "Calling CTOR for OperatorSetConnDSCP"); }

  // noncopyable
  OperatorSetConnDSCP(const OperatorSetConnDSCP &) = delete;
  void operator=(const OperatorSetConnDSCP &)      = delete;

  void initialize(Parser &p) override;

protected:
  void initialize_hooks() override;
  void exec(const Resources &res) const override;

private:
  Value _ds_value;
};

class OperatorSetConnMark : public Operator
{
public:
  OperatorSetConnMark() { Dbg(dbg_ctl, "Calling CTOR for OperatorSetConnMark"); }

  // noncopyable
  OperatorSetConnMark(const OperatorSetConnMark &) = delete;
  void operator=(const OperatorSetConnMark &)      = delete;

  void initialize(Parser &p) override;

protected:
  void initialize_hooks() override;
  void exec(const Resources &res) const override;

private:
  Value _ds_value;
};

class OperatorSetDebug : public Operator
{
public:
  OperatorSetDebug() { Dbg(dbg_ctl, "Calling CTOR for OperatorSetDebug"); }

  // noncopyable
  OperatorSetDebug(const OperatorSetDebug &) = delete;
  void operator=(const OperatorSetDebug &)   = delete;

  void initialize(Parser &p) override;

protected:
  void initialize_hooks() override;
  void exec(const Resources &res) const override;
};

class OperatorSetBody : public Operator
{
public:
  OperatorSetBody() { Dbg(dbg_ctl, "Calling CTOR for OperatorSetBody"); }

  // noncopyable
  OperatorSetBody(const OperatorSetBody &) = delete;
  void operator=(const OperatorSetBody &)  = delete;

  void initialize(Parser &p) override;

protected:
  void initialize_hooks() override;
  void exec(const Resources &res) const override;

private:
  Value _value;
};

class OperatorSetHttpCntl : public Operator
{
public:
  OperatorSetHttpCntl() { Dbg(dbg_ctl, "Calling CTOR for OperatorSetHttpCntl"); }

  // noncopyable
  OperatorSetHttpCntl(const OperatorSetHttpCntl &) = delete;
  void operator=(const OperatorSetHttpCntl &)      = delete;

  void initialize(Parser &p) override;

protected:
  void initialize_hooks() override;
  void exec(const Resources &res) const override;

private:
  bool           _flag = false;
  TSHttpCntlType _cntl_qual;
};
