.. Licensed to the Apache Software Foundation (ASF) under one
   or more contributor license agreements.  See the NOTICE file
  distributed with this work for additional information
  regarding copyright ownership.  The ASF licenses this file
  to you under the Apache License, Version 2.0 (the
  "License"); you may not use this file except in compliance
  with the License.  You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing,
  software distributed under the License is distributed on an
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  KIND, either express or implied.  See the License for the
  specific language governing permissions and limitations
  under the License.

.. _admin-plugins-rate-limit:

Rate Limit Plugin
********************

The :program:`rate_limit` plugin provides basic mechanism for how much
traffic a particular service (remap rule) is allowed to do. Currently,
the only implementation is a limit on how many active client transactions
a service can have. However, it would be easy to refactor this plugin to
allow for adding new limiter policies later on.

The limit counters and queues are per remap rule only, i.e. there is
(currently) no way to group transaction limits from different remap rules
into a single rate limiter.

.. Note::
    This is still work in progress, in particularly the configuration and
    the IP reputation system needs some work. In particular:

    * The remap configuration needs YAML support.
    * We need reloadable configurations.
    * The IP reputation currently only works with the global plugin settings.
    * There is no support for adding allow listed IPs to the IP reputation.

Remap Plugin
------------

All configuration is done via :file:`remap.config`, and the following options
are available:

.. program:: rate-limit

.. option:: --limit

   The maximum number of active client transactions.

.. option:: --queue

   When the limit (above) has been reached, all new transactions are placed
   on a FIFO queue. This option (optional) sets an upper bound on how many
   queued transactions we will allow. When this threshold is reached, all
   additional transactions are immediately served with an error message.

   The queue is effectively disabled if this is set to ``0``, which implies
   that when the transaction limit is reached, we immediately start serving
   error responses.

   The default queue size is ``UINT_MAX``, which is essentially unlimited.

.. option:: --error

   An optional HTTP status error code, to be used together with the
   :option:`--queue` option above. The default is ``429``.

.. option:: --retry

   An optional retry-after value, which if set will cause rejected (e.g. ``429``)
   responses to also include a header ``Retry-After``.

.. option:: --header

   This is an optional HTTP header name, which will be added to the client
   request header IF the transaction was delayed (queued). The value of the
   header is the delay, in milliseconds. This can be useful to for example
   log the delays for later analysis.

   It is recommended that an `@` header is used here, e.g. ``@RateLimit-Delay``,
   since this header will not leave the ATS server instance.

.. option:: --maxage

   An optional ``max-age`` for how long a transaction can sit in the delay queue.
   The value (default 0) is the age in seconds.

.. option:: --prefix

   An optional metric prefix to use instead of the default (plugin.rate_limiter).

.. option:: --tag

   An optional metric tag to use instead of the default. When a tag is not specified
   the plugin will use the scheme, FQDN, and port when it is non-standard. For example
   a default plugin tag might be "https.example.com" or "http.example.com:8080"
   noting that in the latter exampe, the non-standard scheme and port led to
   ":8080" being appended to the string.

.. option:: --conntrack

   This flag tells the limiter that rather than limiting the number of active transactions,
   it should limit the number of active connections. This allows an established connection
   to make any number of transactions, but limits the number of connections that can be
   active at any one time.

   Note that it's highly recommended that you keep a very low ``keep-alive`` timeout for the
   connections that are using this rate limiter.

Global Plugin
-------------

As a global plugin, the rate limiting currently applies only for TLS enabled
connections, based on the SNI from the TLS handshake. As a global plugin we
also have the support of an IP reputation system, see below for configurations.

In addition, the global plugin must be configured via a reloadable YAML
configuration file. The basic use is as::

    rate_limit.so some_config.yaml

.. Note::

    As a global plugin, it's highly recommended to also reduce the Keep-Alive inactive
    timeout for the service(s) controlled by this plugin. This avoids the risk of having
    idle connections consume too many of the available resources. This is easily
    done using e.g. the ``conf_remap`` plugin,
    :ts:cv:`proxy.config.http.keep_alive_no_activity_timeout_in`.


The YAML configuration can have the following format, where the various sections
and nodes are documented below.

   .. code-block:: yaml

      selector:
         - sni: test1.example.com
            limit: 1000
            queue:
               size: 1000
               max-age: 30
            metrics:
               tag: example.com
               prefix: ddos
            ip-rep: main
            exclude: internal
         - sni: test2.example.com
            aliases: [test3.example.com, test4.example.com]
            limit: 100
      ip-rep:
         - name: main
            buckets: 10
            size: 15
            percentage: 90
            max-age: 300
            perma-block:
               limit: 100
               threshold: 1
               max-age: 1800
      lists:
         - name: internal
            cidr:
               - 10.0.0.0/8
               - 192.168.0.0/16


For the top level `selector` node, the following options are available:

.. option:: sni

   The SNI to match for this rate limiter.

.. option:: limit

   The maximum number of active client transactions.

.. option:: aliases

      A list of aliases for the SNI, which will also be matched by this rate limiter.

.. option:: ip-rep

      The name of the IP reputation node to use for this rate limiter. If not
      specified, the IP reputation system is not used for this rate limiter.

.. option:: exclude

      A list of IP CIDR ranges to exclude from any rate limiting. Any IP matching
      this list will not be rate limited, even if the SNI matches.

.. option:: queue

   If enabled, when the limit (above) has been reached, all new connections
   are placed on a FIFO queue. This option sets an upper bound on
   how many queued transactions we will allow. When this threshold is reached,
   all additional connections are immediately errored out in the TLS handshake.

   The queue option can include a `size` and a `max-age` option. The size is
   default to ``UINT_MAX``, which is essentially unlimited. The max-age is
   default to ``0``, which means no age limit.

   No queue is enabled without this configuration directive, but it can also be
   disabled explicitly if the size is set to ``0``.

.. option:: metrics

   This is an optional node, which can be used to configure the metrics for
   this rate limiter. If not specified, no metrics will be added.

   The metrics node can include a `tag` and a `prefix` option. The tag is
   default to the SNI, and the prefix is default to ``plugin.rate_limiter``.

The `lists` node is used to configure IP lists, which can be used to exclude
certain address ranges from the rate limiting. The following options are used:

.. option:: name

   The name of the IP reputation setup, used to refer to it from the rate limiters.

.. option:: cidr

   A list of CIDR ranges to add to this rule. The format is e.g. `10.0.0.0/8`.

The `ip-rep`` node is used to configure the IP reputation system, there can be
zero, one or many IP reputation setups. Each setup is configured with a name,
and the following options:

.. option:: name

   The name of the IP reputation setup, used to refer to it from the rate limiters.

.. option:: buckets

   The number of LRU buckets to use for the IP reputation. A good number here
   is ``10``, which is the default, but can be configured. The reason for the different
   buckets is to account for a pseudo-sorted list of IPs on the frequency seen. Too
   few buckets will not be enough to keep such sorting, rendering the algorithm useless.
   To function in our setup, the number of buckets must be less than ``100``.

.. option:: size

   This is the size of the largest LRU bucket (the ``entry bucket``), ``15`` is a good
   value. This is a power of 2, so ``15`` means the largest LRU can hold ``32768`` entries.
   Note that this option must be bigger then the ``--iprep_buckets`` setting, for the
   bucket halfing to function.

   The default here is ``0``, which means the IP reputation filter is not enabled!

.. option:: percentage

   This is the minimum percentage of the ``limit`` that the pressure must be at, before
   we start blocking IPs. The default is ``0.9`` which means ``90%`` of the limit.

.. option:: max-age

   This is used for aging out entries out of the LRU, the default is ``0`` which means
   no aging happens. Even with no aging, entries will eventually fall out of buckets
   because of the LRU mechanism that kicks in. The aging is here to make sure a spike
   in traffic from an IP doesn't keep the entry for too long in the LRUs.

In addition, there's an optional configuration for the permanently blocking buckets,
`perma-block`. This is a special bucket, which is only used for IPs which have been
blocked for a long time. The configuration for this bucket is:

.. option:: limit

   The minimum number of hits an IP must reach to get moved to the permanent bucket.
   In this bucket, entries will stay for 2x

.. option:: threshold

   This option specifies from which bucket an IP is allowed to move from into the
   perma block bucket. A good value here is likely ``0`` or ``1``, which is very conservative.

.. option:: max-age

   Like above, but only applies to the long term (`perma-block`) bucket. Default is
   ``0``, which means no aging to this bucket is applied.

Metrics
-------
Metric names are generated either using defaults or user-supplied values. In either
case, the format of the metric names is as follows:

   ``prefix.type.tag.metric``

A user can specify their own prefixes and tags, but not types or metrics.

``prefix``
   The default prefix for all metrics is `plugin.rate_limiter`.

``type``
   There are two types of metrics: `sni` and `remap`. Each type corresponds with the
   type of configuration used to generate the metric. The global configuration is for
   rate limiting requests during TLS negotiation, hence, the type of ``sni``. Similarly
   ``remap`` connotes a remap configuration.

``tag``
   By default the metric tag is derived from a description that is set conditionally.
   When configured in global mode, the ``SNI`` argument allows a comma separated list
   of FQDNs that require rate limiting. Each FQDN is associated with an instance of
   the rate limiter, and the description of each limiter is set to the FQDN.

   When configured on a remap, the plugin will generate a description based on the
   configuration. When the scheme and port number are standard, the port is omitted
   from the generated description, however, when the scheme and port combination are
   non-standard, the port is appended. For example, a standard scheme and port would
   lead to a description of ``http.example.com`` or ``https.example.com`` but if a
   non-standard port was used, a description might be ``https.example.com:8443`` or
   ``http.example.com:8080``. This approach allows each limiter to increment metrics
   for the correct remaps.

``metric``
   There are four metrics that may be incremented, depending on which action the plugin takes:

   ============== ===================================================================
   Metric         Definition
   ============== ===================================================================
   ``queued``     Request queued due to being at the limit but under the queue limit.
   ``rejected``   Request rejected due to being over the defined limits.
   ``expired``    Queued connection is too old to be resumed and is rejected.
   ``resumed``    Queued connection is resumed.
   ============== ===================================================================

IP Reputation
-------------

The goal of the IP reputation system is to simply try to identify IPs which are more
likely to be abusive than others. It's not a perfect system, and it relies heavily on
the notion of pressure. The Sieve LRUs are always filled, so you have to make sure that
you only start using them when the system thinks it's under pressure.

The Sieve LRU is a chained set of (configurable) LRUs, each with smaller and smaller
capacity. This essentially adds a notion of partially sorted elements; All IPs in
LRU <n> generally are more active than the IPs in LRU <n+1>. LRU is specially marked
for longer term blocking, only the most abusive elements would end up here.

.. figure:: /static/images/sdk/SieveLRU.png

Examples
--------

This example shows a simple rate limiting of ``128`` concurrently active client
transactions, with a maximum queue size of ``256``. The default of HTTP status
code ``429`` is used when queue is full: ::

    map http://cdn.example.com/ http://some-server.example.com \
      @plugin=rate_limit.so @pparam=--limit=128 @pparam=--queue=256


This example would put a hard transaction (in) limit to 256, with no backoff
queue, and add a header with the transaction delay if it was queued: ::

    map http://cdn.example.com/ http://some-server.example.com \
      @plugin=rate_limit.so @pparam=--limit=256 @pparam=--queue=0 \
      @pparam=--header=@RateLimit-Delay

This final example will limit the active transaction, queue size, and also
add a ``Retry-After`` header once the queue is full and we return a ``429`` error: ::

    map http://cdn.example.com/ http://some-server.example.com \
      @plugin=rate_limit.so @pparam=--limit=256 @pparam=--queue=1024 \
      @pparam=--retry=3600 @pparam=--header=@RateLimit-Delay

In this case, the response would look like this when the queue is full: ::

    HTTP/1.1 429 Too Many Requests
    Date: Fri, 26 Mar 2021 22:42:38 GMT
    Connection: keep-alive
    Server: ATS/10.0.0
    Cache-Control: no-store
    Content-Type: text/html
    Content-Language: en
    Retry-After: 3600
    Content-Length: 207

Metric Examples
---------------
The following examples show the metric names that result from various settings
using a hypothetical domain of example.com with both global and remap configurations.
Note that in this example the remap configuration contains both TLS and non-TLS
remap rules.

Defaults:
::

   proxy.rate_limiter.sni.example.com.queued
   proxy.rate_limiter.sni.example.com.rejected
   proxy.rate_limiter.sni.example.com.expired
   proxy.rate_limiter.sni.example.com.resumed

   proxy.rate_limiter.remap.https.example.com.queued
   proxy.rate_limiter.remap.https.example.com.rejected
   proxy.rate_limiter.remap.https.example.com.expired
   proxy.rate_limiter.remap.https.example.com.resumed

   proxy.rate_limiter.remap.http.example.com.queued
   proxy.rate_limiter.remap.http.example.com.rejected
   proxy.rate_limiter.remap.http.example.com.expired
   proxy.rate_limiter.remap.http.example.com.resumed

Defaults with non-standard scheme+port combinations in the remap rules:
::

   proxy.rate_limiter.sni.example.com.queued
   proxy.rate_limiter.sni.example.com.rejected
   proxy.rate_limiter.sni.example.com.expired
   proxy.rate_limiter.sni.example.com.resumed

   proxy.rate_limiter.remap.https.example.com:8443.queued
   proxy.rate_limiter.remap.https.example.com:8443.rejected
   proxy.rate_limiter.remap.https.example.com:8443.expired
   proxy.rate_limiter.remap.https.example.com:8443.resumed

   proxy.rate_limiter.remap.http.example.com:8080.queued
   proxy.rate_limiter.remap.http.example.com:8080.rejected
   proxy.rate_limiter.remap.http.example.com:8080.expired
   proxy.rate_limiter.remap.http.example.com:8080.resumed

With:
  * ``--prefix=limiter`` on the global configuration
  * ``--tag=tls.example.com`` on the global configuration
  * ``@pparam=--prefix=limiter`` on the remap configurations
  * ``@pparam=--tag=secure.example.com`` on the TLS-enabled remap configuration
  * ``@pparam=--tag=insecure.example.com`` on the non-TLS-enabled remap configuration

::

   limiter.sni.tls.example.com.queued
   limiter.sni.tls.example.com.rejected
   limiter.sni.tls.example.com.expired
   limiter.sni.tls.example.com.resumed

   limiter.remap.secure.example.com.queued
   limiter.remap.secure.example.com.rejected
   limiter.remap.secure.example.com.expired
   limiter.remap.secure.example.com.resumed

   limiter.remap.insecure.example.com.queued
   limiter.remap.insecure.example.com.rejected
   limiter.remap.insecure.example.com.expired
   limiter.remap.insecure.example.com.resumed
