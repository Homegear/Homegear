!function(e){if("object"==typeof exports&&"undefined"!=typeof module)module.exports=e();else if("function"==typeof define&&define.amd)define([],e);else{("undefined"!=typeof window?window:"undefined"!=typeof global?global:"undefined"!=typeof self?self:this).jsonata=e()}}(function(){return function e(t,r,n){function o(a,s){if(!r[a]){if(!t[a]){var u="function"==typeof require&&require;if(!s&&u)return u(a,!0);if(i)return i(a,!0);var c=new Error("Cannot find module '"+a+"'");throw c.code="MODULE_NOT_FOUND",c}var f=r[a]={exports:{}};t[a][0].call(f.exports,function(e){var r=t[a][1][e];return o(r||e)},f,f.exports,e,t,r,n)}return r[a].exports}for(var i="function"==typeof require&&require,a=0;a<n.length;a++)o(n[a]);return o}({1:[function(e,t,r){(function(r){"use strict";function n(e){return e&&e.__esModule?e:{default:e}}var o=n(e("babel-runtime/core-js/symbol")),i=n(e("babel-runtime/core-js/object/is")),a=n(e("babel-runtime/core-js/json/stringify")),s=n(e("babel-runtime/core-js/symbol/iterator")),u=n(e("babel-runtime/core-js/is-iterable")),c=n(e("babel-runtime/core-js/object/keys")),f=n(e("babel-runtime/regenerator")),l=n(e("babel-runtime/core-js/promise")),p=n(e("babel-runtime/core-js/number/is-integer")),d=n(e("babel-runtime/core-js/object/create")),h="function"==typeof o.default&&"symbol"==typeof s.default?function(e){return typeof e}:function(e){return e&&"function"==typeof o.default&&e.constructor===o.default&&e!==o.default.prototype?"symbol":typeof e},b=function(){function e(e){for(var t=1,r=[],n={},o=n;t<e.length;){var i=e.charAt(t);if(":"===i)break;var a=function(){r.push(n),o=n,n={}},s=function(e,t,r,n){for(var o=1,a=t;a<e.length;)if(a++,(i=e.charAt(a))===n){if(0==--o)break}else i===r&&o++;return a};switch(i){case"s":case"n":case"b":case"l":case"o":n.regex="["+i+"m]",n.type=i,a();break;case"a":n.regex="[asnblfom]",n.type=i,n.array=!0,a();break;case"f":n.regex="f",n.type=i,a();break;case"j":n.regex="[asnblom]",n.type=i,a();break;case"x":n.regex="[asnblfom]",n.type=i,a();break;case"-":o.context=!0,o.contextRegex=new RegExp(o.regex),o.regex+="?";break;case"?":case"+":o.regex+=i;break;case"(":var u=s(e,t,"(",")"),c=e.substring(t+1,u);if(-1!==c.indexOf("<"))throw{code:"S0402",stack:(new Error).stack,value:c,offset:t};n.regex="["+c+"m]",n.type="("+c+")",t=u,a();break;case"<":if("a"!==o.type&&"f"!==o.type)throw{code:"S0401",stack:(new Error).stack,value:o.type,offset:t};var f=s(e,t,"<",">");o.subtype=e.substring(t+1,f),t=f}t++}var l="^"+r.map(function(e){return"("+e.regex+")"}).join("")+"$",p=new RegExp(l),d=function(e){var t;if(J(e))t="f";else switch(void 0===e?"undefined":h(e)){case"string":t="s";break;case"number":t="n";break;case"boolean":t="b";break;case"object":t=null===e?"l":Array.isArray(e)?"a":"o";break;case"undefined":t="m"}return t},b=function(e,t){for(var n="^",o=0,i=0;i<r.length;i++){n+=r[i].regex;var a=t.match(n);if(null===a)throw{code:"T0410",stack:(new Error).stack,value:e[o],index:o+1};o=a[0].length}throw{code:"T0410",stack:(new Error).stack,value:e[o],index:o+1}};return{definition:e,validate:function(e,t){var n="";e.forEach(function(e){n+=d(e)});var o=p.exec(n);if(o){var i=[],a=0;return r.forEach(function(r,n){var s=e[a],u=o[n+1];if(""===u)if(r.context){var c=d(t);if(!r.contextRegex.test(c))throw{code:"T0411",stack:(new Error).stack,value:t,index:a+1};i.push(t)}else i.push(s),a++;else u.split("").forEach(function(t){if("a"===r.type){if("m"===t)s=void 0;else{s=e[a];var n=!0;if(void 0!==r.subtype)if("a"!==t&&u!==r.subtype)n=!1;else if("a"===t&&s.length>0){var o=d(s[0]);n=o===r.subtype.charAt(0)&&0===s.filter(function(e){return d(e)!==o}).length}if(!n)throw{code:"T0412",stack:(new Error).stack,value:s,index:a+1,type:r.subtype};"a"!==t&&(s=[s])}i.push(s),a++}else i.push(s),a++})}),i}b(e,n)}}}function t(e){var t=!1;if("number"==typeof e){var r=parseFloat(e);if((t=!isNaN(r))&&!isFinite(r))throw{code:"D1001",value:e,stack:(new Error).stack}}return t}function n(e){var t=!1;return Array.isArray(e)&&(t=0===e.filter(function(e){return"string"!=typeof e}).length),t}function o(e){var r=!1;return Array.isArray(e)&&(r=0===e.filter(function(e){return!t(e)}).length),r}function b(e,t,r){var n,o,i;return f.default.wrap(function(a){for(;;)switch(a.prev=a.next){case 0:(o=r.lookup("__evaluate_entry"))&&o(e,t,r),a.t0=e.type,a.next="path"===a.t0?5:"binary"===a.t0?9:"unary"===a.t0?12:"name"===a.t0?15:"literal"===a.t0?17:"wildcard"===a.t0?19:"descendant"===a.t0?21:"condition"===a.t0?23:"block"===a.t0?26:"bind"===a.t0?29:"regex"===a.t0?32:"function"===a.t0?34:"variable"===a.t0?37:"lambda"===a.t0?39:"partial"===a.t0?41:"apply"===a.t0?44:"sort"===a.t0?47:50;break;case 5:return a.delegateYield(v(e.steps,t,r),"t1",6);case 6:return n=a.t1,n=y(n,e.keepSingletonArray),a.abrupt("break",50);case 9:return a.delegateYield(k(e,t,r),"t2",10);case 10:return n=a.t2,a.abrupt("break",50);case 12:return a.delegateYield(x(e,t,r),"t3",13);case 13:return n=a.t3,a.abrupt("break",50);case 15:return n=w(e,t,r),a.abrupt("break",50);case 17:return n=j(e),a.abrupt("break",50);case 19:return n=E(e,t),a.abrupt("break",50);case 21:return n=A(e,t),a.abrupt("break",50);case 23:return a.delegateYield(I(e,t,r),"t4",24);case 24:return n=a.t4,a.abrupt("break",50);case 26:return a.delegateYield(R(e,t,r),"t5",27);case 27:return n=a.t5,a.abrupt("break",50);case 29:return a.delegateYield(L(e,t,r),"t6",30);case 30:return n=a.t6,a.abrupt("break",50);case 32:return n=$(e),a.abrupt("break",50);case 34:return a.delegateYield(K(e,t,r),"t7",35);case 35:return n=a.t7,a.abrupt("break",50);case 37:return n=C(e,t,r),a.abrupt("break",50);case 39:return n=Q(e,t,r),a.abrupt("break",50);case 41:return a.delegateYield(V(e,t,r),"t8",42);case 42:return n=a.t8,a.abrupt("break",50);case 44:return a.delegateYield(q(e,t,r),"t9",45);case 45:return n=a.t9,a.abrupt("break",50);case 47:return a.delegateYield(U(e,t,r),"t10",48);case 48:return n=a.t10,a.abrupt("break",50);case 50:return!r.lookup("__jsonata_async")||void 0!==n&&null!==n&&"function"==typeof n.then||(n=l.default.resolve(n)),a.next=53,n;case 53:if(n=a.sent,!e.hasOwnProperty("predicate")){a.next=58;break}return a.delegateYield(g(e.predicate,n,r),"t11",56);case 56:n=a.t11,n=y(n);case 58:if(!e.hasOwnProperty("group")){a.next=61;break}return a.delegateYield(D(e.group,n,r),"t12",60);case 60:n=a.t12;case 61:return(i=r.lookup("__evaluate_exit"))&&i(e,t,r,n),a.abrupt("return",n);case 64:case"end":return a.stop()}},ge[0],this)}function v(e,t,r){var n,o,i,a;return f.default.wrap(function(s){for(;;)switch(s.prev=s.next){case 0:n="variable"===e[0].type?[t]:Array.isArray(t)?t:[t],i=0;case 2:if(!(i<e.length)){s.next=17;break}if(a=e[i],0!==i||!a.consarray){s.next=9;break}return s.delegateYield(b(a,n,r),"t0",6);case 6:o=s.t0,s.next=11;break;case 9:return s.delegateYield(_(a,n,r),"t1",10);case 10:o=s.t1;case 11:if(void 0!==o&&0!==o.length){s.next=13;break}return s.abrupt("break",17);case 13:n=o;case 14:i++,s.next=2;break;case 17:return s.abrupt("return",o);case 18:case"end":return s.stop()}},ge[1],this)}function y(e,t){var r;return void 0===e?r=void 0:Array.isArray(e)?1===e.length?r=t?e:e[0]:e.length>1&&(r=e):r=e,r}function _(e,t,r){var n,o,i;return f.default.wrap(function(a){for(;;)switch(a.prev=a.next){case 0:n=[],o=0;case 2:if(!(o<t.length)){a.next=10;break}return a.delegateYield(b(e,t[o],r),"t0",4);case 4:i=a.t0,Array.isArray(i)&&"["!==e.value||e.consarray||(i=[i]),i.forEach(function(e){void 0!==e&&n.push(e)});case 7:o++,a.next=2;break;case 10:return a.abrupt("return",n);case 11:case"end":return a.stop()}},ge[2],this)}function g(e,r,n){var o,i,a,s,u;return f.default.wrap(function(c){for(;;)switch(c.prev=c.next){case 0:o=r,i=[],a=0;case 3:if(!(a<e.length)){c.next=20;break}if(s=e[a],Array.isArray(o)||(o=[o]),i=[],"literal"!==s.type||!t(s.value)){c.next=14;break}u=s.value,(0,p.default)(u)||(u=Math.floor(u)),u<0&&(u=o.length+u),i=o[u],c.next=16;break;case 14:return c.delegateYield(m(s,o,n),"t0",15);case 15:i=c.t0;case 16:o=i;case 17:a++,c.next=3;break;case 20:return c.abrupt("return",i);case 21:case"end":return c.stop()}},ge[3],this)}function m(e,r,n){var i,a,s,u;return f.default.wrap(function(c){for(;;)switch(c.prev=c.next){case 0:i=[],a=0;case 2:if(!(a<r.length)){c.next=11;break}return s=r[a],c.delegateYield(b(e,s,n),"t0",5);case 5:t(u=c.t0)&&(u=[u]),o(u)?u.forEach(function(e){(0,p.default)(e)||(e=Math.floor(e)),e<0&&(e=r.length+e),e===a&&i.push(s)}):se(u)&&i.push(s);case 8:a++,c.next=2;break;case 11:return c.abrupt("return",i);case 12:case"end":return c.stop()}},ge[4],this)}function k(e,t,r){var n,o,i,a;return f.default.wrap(function(s){for(;;)switch(s.prev=s.next){case 0:return s.delegateYield(b(e.lhs,t,r),"t0",1);case 1:return o=s.t0,s.delegateYield(b(e.rhs,t,r),"t1",3);case 3:i=s.t1,a=e.value,s.prev=5,s.t2=a,s.next="+"===s.t2?9:"-"===s.t2?9:"*"===s.t2?9:"/"===s.t2?9:"%"===s.t2?9:"="===s.t2?11:"!="===s.t2?11:"<"===s.t2?11:"<="===s.t2?11:">"===s.t2?11:">="===s.t2?11:"&"===s.t2?13:"and"===s.t2?15:"or"===s.t2?15:".."===s.t2?17:"in"===s.t2?19:21;break;case 9:return n=T(o,i,a),s.abrupt("break",21);case 11:return n=P(o,i,a),s.abrupt("break",21);case 13:return n=N(o,i),s.abrupt("break",21);case 15:return n=Y(o,i,a),s.abrupt("break",21);case 17:return n=F(o,i),s.abrupt("break",21);case 19:return n=M(o,i),s.abrupt("break",21);case 21:s.next=28;break;case 23:throw s.prev=23,s.t3=s.catch(5),s.t3.position=e.position,s.t3.token=a,s.t3;case 28:return s.abrupt("return",n);case 29:case"end":return s.stop()}},ge[5],this,[[5,23]])}function x(e,r,n){var o,i,a,s;return f.default.wrap(function(u){for(;;)switch(u.prev=u.next){case 0:u.t0=e.value,u.next="-"===u.t0?3:"["===u.t0?11:"{"===u.t0?22:25;break;case 3:return u.delegateYield(b(e.expression,r,n),"t1",4);case 4:if(o=u.t1,!t(o)){u.next=9;break}o=-o,u.next=10;break;case 9:throw{code:"D1002",stack:(new Error).stack,position:e.position,token:e.value,value:o};case 10:return u.abrupt("break",25);case 11:o=[],i=0;case 13:if(!(i<e.lhs.length)){u.next=21;break}return a=e.lhs[i],u.delegateYield(b(a,r,n),"t2",16);case 16:void 0!==(s=u.t2)&&("["===a.value?o.push(s):o=pe(o,s));case 18:i++,u.next=13;break;case 21:return u.abrupt("break",25);case 22:return u.delegateYield(D(e,r,n),"t3",23);case 23:return o=u.t3,u.abrupt("break",25);case 25:return u.abrupt("return",o);case 26:case"end":return u.stop()}},ge[6],this)}function w(e,t,r){var n;if(Array.isArray(t)){n=[];for(var o=0;o<t.length;o++){var i=w(e,t[o],r);void 0!==i&&n.push(i)}}else null!==t&&"object"===(void 0===t?"undefined":h(t))&&(n=t[e.value]);return n=y(n)}function j(e){return e.value}function E(e,t){var r=[];return null!==t&&"object"===(void 0===t?"undefined":h(t))&&(0,c.default)(t).forEach(function(e){var n=t[e];Array.isArray(n)?(n=S(n),r=pe(r,n)):r.push(n)}),y(r)}function S(e,t){return void 0===t&&(t=[]),Array.isArray(e)?e.forEach(function(e){S(e,t)}):t.push(e),t}function A(e,t){var r,n=[];return void 0!==t&&(O(t,n),r=1===n.length?n[0]:n),r}function O(e,t){Array.isArray(e)||t.push(e),Array.isArray(e)?e.forEach(function(e){O(e,t)}):null!==e&&"object"===(void 0===e?"undefined":h(e))&&(0,c.default)(e).forEach(function(r){O(e[r],t)})}function T(e,r,n){var o;if(void 0===e||void 0===r)return o;if(!t(e))throw{code:"T2001",stack:(new Error).stack,value:e};if(!t(r))throw{code:"T2002",stack:(new Error).stack,value:r};switch(n){case"+":o=e+r;break;case"-":o=e-r;break;case"*":o=e*r;break;case"/":o=e/r;break;case"%":o=e%r}return o}function P(e,t,r){var n,o=void 0===e?"undefined":h(e),i=void 0===t?"undefined":h(t);if("undefined"===o||"undefined"===i)return!1;var a=function(){if("string"!==o&&"number"!==o||"string"!==i&&"number"!==i)throw{code:"T2010",stack:(new Error).stack,value:"string"!==o&&"number"!==o?e:t};if(o!==i)throw{code:"T2009",stack:(new Error).stack,value:e,value2:t}};switch(r){case"=":n=e===t;break;case"!=":n=e!==t;break;case"<":a(),n=e<t;break;case"<=":a(),n=e<=t;break;case">":a(),n=e>t;break;case">=":a(),n=e>=t}return n}function M(e,t){var r=!1;if(void 0===e||void 0===t)return!1;Array.isArray(t)||(t=[t]);for(var n=0;n<t.length;n++)if(t[n]===e){r=!0;break}return r}function Y(e,t,r){var n;switch(r){case"and":n=se(e)&&se(t);break;case"or":n=se(e)||se(t)}return n}function N(e,t){var r="",n="";return void 0!==e&&(r=ie(e)),void 0!==t&&(n=ie(t)),r.concat(n)}function D(e,t,r){var n,o,i,a,s,u,c,l,p;return f.default.wrap(function(d){for(;;)switch(d.prev=d.next){case 0:n={},o={},Array.isArray(t)||(t=[t]),i=[],a=0;case 5:if(!(a<t.length)){d.next=18;break}s=0;case 7:if(!(s<e.lhs.length)){d.next=15;break}return d.t0=i,d.delegateYield(b(e.lhs[s][0],t[a],r),"t1",10);case 10:d.t2=d.t1,d.t0.push.call(d.t0,d.t2);case 12:s++,d.next=7;break;case 15:a++,d.next=5;break;case 18:u=i.entries(),t.forEach(function(t){e.lhs.forEach(function(r){var n=u.next().value[1];if("string"!=typeof n)throw{code:"T1003",stack:(new Error).stack,position:e.position,value:n};var i={data:t,expr:r[1]};o.hasOwnProperty(n)?o[n].data=pe(o[n].data,t):o[n]=i})}),i=[],d.t3=f.default.keys(o);case 22:if((d.t4=d.t3()).done){d.next=31;break}return c=d.t4.value,l=o[c],d.t5=i,d.delegateYield(b(l.expr,l.data,r),"t6",27);case 27:d.t7=d.t6,d.t5.push.call(d.t5,d.t7),d.next=22;break;case 31:u=i.entries();for(c in o)p=u.next().value[1],n[c]=p;return d.abrupt("return",n);case 34:case"end":return d.stop()}},ge[7],this)}function F(e,t){var r;if(void 0===e||void 0===t)return r;if(e>t)return r;if(!(0,p.default)(e))throw{code:"T2003",stack:(new Error).stack,value:e};if(!(0,p.default)(t))throw{code:"T2004",stack:(new Error).stack,value:t};r=new Array(t-e+1);for(var n=e,o=0;n<=t;n++,o++)r[o]=n;return r}function L(e,t,r){var n;return f.default.wrap(function(o){for(;;)switch(o.prev=o.next){case 0:return o.delegateYield(b(e.rhs,t,r),"t0",1);case 1:if(n=o.t0,"variable"===e.lhs.type){o.next=4;break}throw{code:"D2005",stack:(new Error).stack,position:e.position,token:e.value,value:"path"===e.lhs.type?e.lhs.steps[0].value:e.lhs.value};case 4:return r.bind(e.lhs.value,n),o.abrupt("return",n);case 6:case"end":return o.stop()}},ge[8],this)}function I(e,t,r){var n,o;return f.default.wrap(function(i){for(;;)switch(i.prev=i.next){case 0:return i.delegateYield(b(e.condition,t,r),"t0",1);case 1:if(o=i.t0,!se(o)){i.next=7;break}return i.delegateYield(b(e.then,t,r),"t1",4);case 4:n=i.t1,i.next=10;break;case 7:if(void 0===e.else){i.next=10;break}return i.delegateYield(b(e.else,t,r),"t2",9);case 9:n=i.t2;case 10:return i.abrupt("return",n);case 11:case"end":return i.stop()}},ge[9],this)}function R(e,t,r){var n,o,i;return f.default.wrap(function(a){for(;;)switch(a.prev=a.next){case 0:o=ve(r),i=0;case 2:if(!(i<e.expressions.length)){a.next=8;break}return a.delegateYield(b(e.expressions[i],t,o),"t0",4);case 4:n=a.t0;case 5:i++,a.next=2;break;case 8:return a.abrupt("return",n);case 9:case"end":return a.stop()}},ge[10],this)}function $(e){e.value.lastIndex=0;return function t(r){var n,o=e.value,i=o.exec(r);if(null!==i){if(n={match:i[0],start:i.index,end:i.index+i[0].length,groups:[]},i.length>1)for(var a=1;a<i.length;a++)n.groups.push(i[a]);n.next=function(){if(!(o.lastIndex>=r.length)){var n=t(r);if(n&&""===n.match&&o.lastIndex===e.value.lastIndex)throw{code:"D1004",stack:(new Error).stack,position:e.position,value:e.value.source};return n}}}return n}}function C(e,t,r){return""===e.value?t:r.lookup(e.value)}function U(e,t,r){var n,o,i;return f.default.wrap(function(a){for(;;)switch(a.prev=a.next){case 0:return a.delegateYield(b(e.lhs,t,r),"t0",1);case 1:return o=a.t0,i=function(t,n){for(var o=0,i=0;0===o&&i<e.rhs.length;i++){var a=e.rhs[i],s=G(a.expression,t,r),u=G(a.expression,n,r),c=void 0===s?"undefined":h(s),f=void 0===u?"undefined":h(u);if("undefined"!==c)if("undefined"!==f){if("string"!==c&&"number"!==c||"string"!==f&&"number"!==f)throw{code:"T2008",stack:(new Error).stack,position:e.position,value:"string"!==c&&"number"!==c?s:u};if(c!==f)throw{code:"T2007",stack:(new Error).stack,position:e.position,value:s,value2:u};s!==u&&(o=s<u?-1:1,!0===a.descending&&(o=-o))}else o=-1;else o="undefined"===f?0:1}return 1===o},n=be(o,i),a.abrupt("return",n);case 5:case"end":return a.stop()}},ge[11],this)}function G(e,t,r){for(var n=b(e,t,r),o=n.next();!o.done;)o=n.next(o.value);return o.value}function q(e,t,r){var n,o,i;return f.default.wrap(function(a){for(;;)switch(a.prev=a.next){case 0:return a.delegateYield(b(e.lhs,t,r),"t0",1);case 1:if(o=a.t0,"function"!==e.rhs.type){a.next=7;break}return a.delegateYield(K(e.rhs,t,r,{context:o}),"t1",4);case 4:n=a.t1,a.next=18;break;case 7:return a.delegateYield(b(e.rhs,t,r),"t2",8);case 8:if(i=a.t2,J(i)){a.next=11;break}throw{code:"T2006",stack:(new Error).stack,position:e.position,value:i};case 11:if(!J(o)){a.next=16;break}return a.delegateYield(z(Ee,[o,i],r,null),"t3",13);case 13:n=a.t3,a.next=18;break;case 16:return a.delegateYield(z(i,[o],r,null),"t4",17);case 17:n=a.t4;case 18:return a.abrupt("return",n);case 19:case"end":return a.stop()}},ge[12],this)}function J(e){return e&&(!0===e._jsonata_function||!0===e._jsonata_lambda)||"function"==typeof e}function W(e){return e&&!0===e._jsonata_lambda}function B(e){return"object"===(void 0===e?"undefined":h(e))&&(0,u.default)(e)&&"function"==typeof e[s.default]&&"next"in e&&"function"==typeof e.next}function K(e,t,r,n){var o,i,a,s;return f.default.wrap(function(u){for(;;)switch(u.prev=u.next){case 0:i=[],a=0;case 2:if(!(a<e.arguments.length)){u.next=10;break}return u.t0=i,u.delegateYield(b(e.arguments[a],t,r),"t1",5);case 5:u.t2=u.t1,u.t0.push.call(u.t0,u.t2);case 7:a++,u.next=2;break;case 10:return n&&i.unshift(n.context),u.delegateYield(b(e.procedure,t,r),"t3",12);case 12:if(void 0!==(s=u.t3)||"path"!==e.procedure.type||!r.lookup(e.procedure.steps[0].value)){u.next=15;break}throw{code:"T1005",stack:(new Error).stack,position:e.position,token:e.procedure.steps[0].value};case 15:return u.prev=15,u.delegateYield(z(s,i,t),"t4",17);case 17:o=u.t4,u.next=25;break;case 20:throw u.prev=20,u.t5=u.catch(15),u.t5.position=e.position,u.t5.token="path"===e.procedure.type?e.procedure.steps[0].value:e.procedure.value,u.t5;case 25:return u.abrupt("return",o);case 26:case"end":return u.stop()}},ge[13],this,[[15,20]])}function z(e,t,r){var n,o,i,a;return f.default.wrap(function(s){for(;;)switch(s.prev=s.next){case 0:return s.delegateYield(H(e,t,r),"t0",1);case 1:n=s.t0;case 2:if(!W(n)||!0!==n.thunk){s.next=19;break}return s.delegateYield(b(n.body.procedure,n.input,n.environment),"t1",4);case 4:o=s.t1,i=[],a=0;case 7:if(!(a<n.body.arguments.length)){s.next=15;break}return s.t2=i,s.delegateYield(b(n.body.arguments[a],n.input,n.environment),"t3",10);case 10:s.t4=s.t3,s.t2.push.call(s.t2,s.t4);case 12:a++,s.next=7;break;case 15:return s.delegateYield(H(o,i,r),"t5",16);case 16:n=s.t5,s.next=2;break;case 19:return s.abrupt("return",n);case 20:case"end":return s.stop()}},ge[14],this)}function H(e,t,r){var n,o;return f.default.wrap(function(i){for(;;)switch(i.prev=i.next){case 0:if(o=t,e&&(o=X(e.signature,t,r)),!W(e)){i.next=7;break}return i.delegateYield(Z(e,o),"t0",4);case 4:n=i.t0,i.next=19;break;case 7:if(!e||!0!==e._jsonata_function){i.next=14;break}if(n=e.implementation.apply(r,o),!B(n)){i.next=12;break}return i.delegateYield(n,"t1",11);case 11:n=i.t1;case 12:i.next=19;break;case 14:if("function"!=typeof e){i.next=18;break}n=e.apply(r,o),i.next=19;break;case 18:throw{code:"T1006",stack:(new Error).stack};case 19:return i.abrupt("return",n);case 20:case"end":return i.stop()}},ge[15],this)}function Q(e,t,r){var n={_jsonata_lambda:!0,input:t,environment:r,arguments:e.arguments,signature:e.signature,body:e.body};return!0===e.thunk&&(n.thunk=!0),n}function V(e,t,r){var n,o,i,a,s;return f.default.wrap(function(u){for(;;)switch(u.prev=u.next){case 0:o=[],i=0;case 2:if(!(i<e.arguments.length)){u.next=15;break}if("operator"!==(a=e.arguments[i]).type||"?"!==a.value){u.next=8;break}o.push(a),u.next=12;break;case 8:return u.t0=o,u.delegateYield(b(a,t,r),"t1",10);case 10:u.t2=u.t1,u.t0.push.call(u.t0,u.t2);case 12:i++,u.next=2;break;case 15:return u.delegateYield(b(e.procedure,t,r),"t3",16);case 16:if(void 0!==(s=u.t3)||"path"!==e.procedure.type||!r.lookup(e.procedure.steps[0].value)){u.next=19;break}throw{code:"T1007",stack:(new Error).stack,position:e.position,token:e.procedure.steps[0].value};case 19:if(!W(s)){u.next=23;break}n=ee(s,o),u.next=32;break;case 23:if(!s||!0!==s._jsonata_function){u.next=27;break}n=te(s.implementation,o),u.next=32;break;case 27:if("function"!=typeof s){u.next=31;break}n=te(s,o),u.next=32;break;case 31:throw{code:"T1008",stack:(new Error).stack,position:e.position,token:"path"===e.procedure.type?e.procedure.steps[0].value:e.procedure.value};case 32:return u.abrupt("return",n);case 33:case"end":return u.stop()}},ge[16],this)}function X(e,t,r){return void 0===e?t:e.validate(t,r)}function Z(e,t){var r,n;return f.default.wrap(function(o){for(;;)switch(o.prev=o.next){case 0:if(n=ve(e.environment),e.arguments.forEach(function(e,r){n.bind(e.value,t[r])}),"function"!=typeof e.body){o.next=7;break}return o.delegateYield(re(e.body,n),"t0",4);case 4:r=o.t0,o.next=9;break;case 7:return o.delegateYield(b(e.body,e.input,n),"t1",8);case 8:r=o.t1;case 9:return o.abrupt("return",r);case 10:case"end":return o.stop()}},ge[17],this)}function ee(e,t){var r=ve(e.environment),n=[];return e.arguments.forEach(function(e,o){var i=t[o];i&&"operator"===i.type&&"?"===i.value?n.push(e):r.bind(e.value,i)}),{_jsonata_lambda:!0,input:e.input,environment:r,arguments:n,body:e.body}}function te(e,t){var r=ne(e),n="function("+(r=r.map(function(e){return"$"+e.trim()})).join(", ")+"){ _ }",o=we(n);return o.body=e,ee(o,t)}function re(e,t){var r,n,o;return f.default.wrap(function(i){for(;;)switch(i.prev=i.next){case 0:if(r=ne(e),n=r.map(function(e){return t.lookup(e.trim())}),o=e.apply(null,n),!B(o)){i.next=6;break}return i.delegateYield(o,"t0",5);case 5:o=i.t0;case 6:return i.abrupt("return",o);case 7:case"end":return i.stop()}},ge[18],this)}function ne(e){var t=e.toString();return/\(([^)]*)\)/.exec(t)[1].split(",")}function oe(t,r){var n={_jsonata_function:!0,implementation:t};return void 0!==r&&(n.signature=e(r)),n}function ie(e){if(void 0!==e){var r;if("string"==typeof e)r=e;else if(J(e))r="";else{if("number"==typeof e&&!isFinite(e))throw{code:"D3001",value:e,stack:(new Error).stack};r=(0,a.default)(e,function(e,r){return void 0!==r&&null!==r&&r.toPrecision&&t(r)?Number(r.toPrecision(13)):r&&J(r)?"":r})}return r}}function ae(e,t,r,n){var o,i,a,s,u,c,l;return f.default.wrap(function(f){for(;;)switch(f.prev=f.next){case 0:if(void 0!==e){f.next=2;break}return f.abrupt("return",void 0);case 2:if(""!==t){f.next=4;break}throw{code:"D3010",stack:(new Error).stack,value:t,index:2};case 4:if(!(n<0)){f.next=6;break}throw{code:"D3011",stack:(new Error).stack,value:n,index:4};case 6:if(o="string"==typeof r?function(e){for(var t="",n=0,o=r.indexOf("$",n);-1!==o&&n<r.length;){t+=r.substring(n,o),n=o+1;var i=r.charAt(n);if("$"===i)t+="$",n++;else if("0"===i)t+=e.match,n++;else{var a;if(a=0===e.groups.length?1:Math.floor(Math.log(e.groups.length)*Math.LOG10E)+1,o=parseInt(r.substring(n,n+a),10),a>1&&o>e.groups.length&&(o=parseInt(r.substring(n,n+a-1),10)),isNaN(o))t+="$";else{if(e.groups.length>0){var s=e.groups[o-1];void 0!==s&&(t+=s)}n+=o.toString().length}}o=r.indexOf("$",n)}return t+=r.substring(n)}:r,i="",a=0,!(void 0===n||n>0)){f.next=39;break}if(s=0,"string"!=typeof t){f.next=17;break}for(u=e.indexOf(t,a);-1!==u&&(void 0===n||s<n);)i+=e.substring(a,u),i+=r,a=u+t.length,s++,u=e.indexOf(t,a);i+=e.substring(a),f.next=37;break;case 17:if(void 0===(c=t(e))){f.next=36;break}case 19:if(void 0===c||!(void 0===n||s<n)){f.next=33;break}return i+=e.substring(a,c.start),f.delegateYield(z(o,[c],null),"t0",22);case 22:if("string"!=typeof(l=f.t0)){f.next=27;break}i+=l,f.next=28;break;case 27:throw{code:"D3012",stack:(new Error).stack,value:l};case 28:a=c.start+c.match.length,s++,c=c.next(),f.next=19;break;case 33:i+=e.substring(a),f.next=37;break;case 36:i=e;case 37:f.next=40;break;case 39:i=e;case 40:return f.abrupt("return",i);case 41:case"end":return f.stop()}},ge[19],this)}function se(e){if(void 0!==e){var r=!1;return Array.isArray(e)?1===e.length?r=se(e[0]):e.length>1&&(r=e.filter(function(e){return se(e)}).length>0):"string"==typeof e?e.length>0&&(r=!0):t(e)?0!==e&&(r=!0):null!==e&&"object"===(void 0===e?"undefined":h(e))?(0,c.default)(e).length>0&&(W(e)||e._jsonata_function||(r=!0)):"boolean"==typeof e&&!0===e&&(r=!0),r}}function ue(e,t){var r,n,o,i,a;return f.default.wrap(function(s){for(;;)switch(s.prev=s.next){case 0:if(void 0!==e){s.next=2;break}return s.abrupt("return",void 0);case 2:r=[],n=0;case 4:if(!(n<e.length)){s.next=15;break}return o=[e[n]],(i="function"==typeof t?t.length:!0===t._jsonata_function?t.implementation.length:t.arguments.length)>=2&&o.push(n),i>=3&&o.push(e),s.delegateYield(z(t,o,null),"t0",10);case 10:void 0!==(a=s.t0)&&r.push(a);case 12:n++,s.next=4;break;case 15:return s.abrupt("return",r);case 16:case"end":return s.stop()}},ge[20],this)}function ce(e,t){var r,n,o,i;return f.default.wrap(function(a){for(;;)switch(a.prev=a.next){case 0:if(void 0!==e){a.next=2;break}return a.abrupt("return",void 0);case 2:for(r=[],n=function(e,r,n){for(var o=z(t,[e,r,n],null),i=o.next();!i.done;)i=o.next(i.value);return i.value},o=0;o<e.length;o++)i=e[o],se(n(i,o,e))&&r.push(i);return a.abrupt("return",r);case 6:case"end":return a.stop()}},ge[21],this)}function fe(e,t,r){var n,o;return f.default.wrap(function(i){for(;;)switch(i.prev=i.next){case 0:if(void 0!==e){i.next=2;break}return i.abrupt("return",void 0);case 2:if(2===t.length||!0===t._jsonata_function&&2===t.implementation.length||2===t.arguments.length){i.next=4;break}throw{stack:(new Error).stack,code:"D3050",index:1};case 4:void 0===r&&e.length>0?(n=e[0],o=1):(n=r,o=0);case 5:if(!(o<e.length)){i.next=11;break}return i.delegateYield(z(t,[n,e[o]],null),"t0",7);case 7:n=i.t0,o++,i.next=5;break;case 11:return i.abrupt("return",n);case 12:case"end":return i.stop()}},ge[22],this)}function le(e){var t=[];if(Array.isArray(e)){var r={};e.forEach(function(e){var t=le(e);Array.isArray(t)&&t.forEach(function(e){r[e]=!0})}),t=le(r)}else null===e||"object"!==(void 0===e?"undefined":h(e))||W(e)?t=void 0:0===(t=(0,c.default)(e)).length&&(t=void 0);return t}function pe(e,t){return void 0===e?t:void 0===t?e:(Array.isArray(e)||(e=[e]),Array.isArray(t)||(t=[t]),Array.prototype.push.apply(e,t),e)}function de(e){var t=[];if(Array.isArray(e))e.forEach(function(e){t=pe(t,de(e))});else if(null===e||"object"!==(void 0===e?"undefined":h(e))||W(e))t=e;else for(var r in e){var n={};n[r]=e[r],t.push(n)}return t}function he(e,t){var r,n,o;return f.default.wrap(function(i){for(;;)switch(i.prev=i.next){case 0:r=[],i.t0=f.default.keys(e);case 2:if((i.t1=i.t0()).done){i.next=11;break}return n=i.t1.value,o=[e[n],n],i.t2=r,i.delegateYield(z(t,o,null),"t3",7);case 7:i.t4=i.t3,i.t2.push.call(i.t2,i.t4),i.next=2;break;case 11:return i.abrupt("return",r);case 12:case"end":return i.stop()}},ge[23],this)}function be(e,t){if(void 0!==e){if(e.length<=1)return e;var r;if(void 0===t){if(!o(e)&&!n(e))throw{stack:(new Error).stack,code:"D3070",index:1};r=function(e,t){return e>t}}else r="function"==typeof t?t:function(e,r){for(var n=z(t,[e,r],null),o=n.next();!o.done;)o=n.next(o.value);return o.value};var i=function(e,t){var n=[];return function e(t,n,o){0===n.length?Array.prototype.push.apply(t,o):0===o.length?Array.prototype.push.apply(t,n):r(n[0],o[0])?(t.push(o[0]),e(t,n,o.slice(1))):(t.push(n[0]),e(t,n.slice(1),o))}(n,e,t),n};return function e(t){if(t.length<=1)return t;var r=Math.floor(t.length/2),n=t.slice(0,r),o=t.slice(r);return n=e(n),o=e(o),i(n,o)}(e)}}function ve(e){var t={};return{bind:function(e,r){t[e]=r},lookup:function(r){var n;return t.hasOwnProperty(r)?n=t[r]:e&&(n=e.lookup(r)),n}}}function ye(e){var t="Unknown error";void 0!==e.message&&(t=e.message);var r=Se[e.code];return void 0!==r&&(t=(t=r.replace(/\{\{\{([^}]+)}}}/g,function(){return e[arguments[1]]})).replace(/\{\{([^}]+)}}/g,function(){return(0,a.default)(e[arguments[1]])})),t}function _e(e){var t;try{t=we(e)}catch(e){throw e.message=ye(e),e}var r=ve(je);return{evaluate:function(e,n,o){if(void 0!==n){var i;i=ve(r);for(var a in n)i.bind(a,n[a])}else i=r;i.bind("$",e);var s=(new Date).toJSON();i.bind("now",oe(function(){return s},"<:s>"));var u,c;if("function"==typeof o){i.bind("__jsonata_async",!0);var f=function e(t){(u=c.next(t)).done?o(null,u.value):u.value.then(e).catch(function(e){e.message=ye(e),o(e,null)})};c=b(t,e,i),(u=c.next()).value.then(f)}else try{for(c=b(t,e,i),u=c.next();!u.done;)u=c.next(u.value);return u.value}catch(e){throw e.message=ye(e),e}},assign:function(e,t){r.bind(e,t)},registerFunction:function(e,t,n){var o=oe(t,n);r.bind(e,o)}}}var ge=[b,v,_,g,m,k,x,D,L,I,R,U,q,K,z,H,V,Z,re,ae,ue,ce,fe,he].map(f.default.mark),me={".":75,"[":80,"]":0,"{":70,"}":0,"(":80,")":0,",":0,"@":75,"#":70,";":80,":":80,"?":20,"+":50,"-":50,"*":60,"/":60,"%":60,"|":20,"=":40,"<":40,">":40,"`":80,"^":40,"**":60,"..":20,":=":10,"!=":40,"<=":40,">=":40,"~>":40,and:30,or:25,in:40,"&":50,"!":0,"~":0},ke={'"':'"',"\\":"\\","/":"/",b:"\b",f:"\f",n:"\n",r:"\r",t:"\t"},xe=function(e){var t=0,r=e.length,n=function(e,r){return{type:e,value:r,position:t}},o=function(){for(var n,o,i=t,a=0;t<r;){var s=e.charAt(t);if("/"===s&&"\\"!==e.charAt(t-1)&&0===a){if(""===(n=e.substring(i,t)))throw{code:"S0301",stack:(new Error).stack,position:t};for(t++,s=e.charAt(t),i=t;"i"===s||"m"===s;)t++,s=e.charAt(t);return o=e.substring(i,t)+"g",new RegExp(n,o)}"("!==s&&"["!==s&&"{"!==s||"\\"===e.charAt(t-1)||a++,")"!==s&&"]"!==s&&"}"!==s||"\\"===e.charAt(t-1)||a--,t++}throw{code:"S0302",stack:(new Error).stack,position:t}};return function(i){if(t>=r)return null;for(var a=e.charAt(t);t<r&&" \t\n\r\v".indexOf(a)>-1;)t++,a=e.charAt(t);if(!0!==i&&"/"===a)return t++,n("regex",o());if("."===a&&"."===e.charAt(t+1))return t+=2,n("operator","..");if(":"===a&&"="===e.charAt(t+1))return t+=2,n("operator",":=");if("!"===a&&"="===e.charAt(t+1))return t+=2,n("operator","!=");if(">"===a&&"="===e.charAt(t+1))return t+=2,n("operator",">=");if("<"===a&&"="===e.charAt(t+1))return t+=2,n("operator","<=");if("*"===a&&"*"===e.charAt(t+1))return t+=2,n("operator","**");if("~"===a&&">"===e.charAt(t+1))return t+=2,n("operator","~>");if(me.hasOwnProperty(a))return t++,n("operator",a);if('"'===a||"'"===a){var s=a;t++;for(var u="";t<r;){if("\\"===(a=e.charAt(t)))if(t++,a=e.charAt(t),ke.hasOwnProperty(a))u+=ke[a];else{if("u"!==a)throw{code:"S0103",stack:(new Error).stack,position:t,token:a};var c=e.substr(t+1,4);if(!/^[0-9a-fA-F]+$/.test(c))throw{code:"S0104",stack:(new Error).stack,position:t};var f=parseInt(c,16);u+=String.fromCharCode(f),t+=4}else{if(a===s)return t++,n("string",u);u+=a}t++}throw{code:"S0101",stack:(new Error).stack,position:t}}var l=/^-?(0|([1-9][0-9]*))(\.[0-9]+)?([Ee][-+]?[0-9]+)?/.exec(e.substring(t));if(null!==l){var p=parseFloat(l[0]);if(!isNaN(p)&&isFinite(p))return t+=l[0].length,n("number",p);throw{code:"S0102",stack:(new Error).stack,position:t,token:l[0]}}for(var d,h,b=t;;)if(d=e.charAt(b),b===r||" \t\n\r\v".indexOf(d)>-1||me.hasOwnProperty(d)){if("$"===e.charAt(t))return h=e.substring(t+1,b),t=b,n("variable",h);switch(h=e.substring(t,b),t=b,h){case"or":case"in":case"and":return n("operator",h);case"true":return n("value",!0);case"false":return n("value",!1);case"null":return n("value",null);default:return t===r&&""===h?null:n("name",h)}}else b++}},we=function(r){var n,o,i={},a={nud:function(){return this}},s=function(e,t){var r=i[e];return t=t||0,r?t>=r.lbp&&(r.lbp=t):((r=(0,d.default)(a)).id=r.value=e,r.lbp=t,i[e]=r),r},u=function(e,t){if(e&&n.id!==e){var a;throw a="(end)"===n.id?"S0203":"S0202",{code:a,stack:(new Error).stack,position:n.position,token:n.id,value:e}}var s=o(t);if(null===s)return n=i["(end)"],n.position=r.length,n;var u,c=s.value,f=s.type;switch(f){case"name":case"variable":u=i["(name)"];break;case"operator":if(!(u=i[c]))throw{code:"S0204",stack:(new Error).stack,position:s.position,token:c};break;case"string":case"number":case"value":f="literal",u=i["(literal)"];break;case"regex":f="regex",u=i["(regex)"];break;default:throw{code:"S0205",stack:(new Error).stack,position:s.position,token:c}}return n=(0,d.default)(u),n.value=c,n.type=f,n.position=s.position,n},c=function(e){var t,r=n;for(u(null,!0),t=r.nud();e<n.lbp;)r=n,u(),t=r.led(t);return t},f=function(e,t,r){var n=t||me[e],o=s(e,n);return o.led=r||function(e){return this.lhs=e,this.rhs=c(n),this.type="binary",this},o},l=function(e,t){var r=s(e);return r.nud=t||function(){return this.expression=c(70),this.type="unary",this},r};s("(end)"),s("(name)"),s("(literal)"),s("(regex)"),s(":"),s(";"),s(","),s(")"),s("]"),s("}"),s(".."),f("."),f("+"),f("-"),f("*"),f("/"),f("%"),f("="),f("<"),f(">"),f("!="),f("<="),f(">="),f("&"),f("and"),f("or"),f("in"),function(e,t,r){var n=t||me[e],o=s(e,n);o.led=r||function(e){return this.lhs=e,this.rhs=c(n-1),this.type="binary",this}}(":="),l("-"),f("~>"),l("*",function(){return this.type="wildcard",this}),l("**",function(){return this.type="descendant",this}),f("(",me["("],function(t){if(this.procedure=t,this.type="function",this.arguments=[],")"!==n.id)for(;"operator"===n.type&&"?"===n.id?(this.type="partial",this.arguments.push(n),u("?")):this.arguments.push(c(0)),","===n.id;)u(",");if(u(")",!0),"name"===t.type&&("function"===t.value||"λ"===t.value)){if(this.arguments.forEach(function(e,t){if("variable"!==e.type)throw{code:"S0208",stack:(new Error).stack,position:e.position,token:e.value,value:t+1}}),this.type="lambda","<"===n.id){for(var r=n.position,o=1,i="<";o>0&&"{"!==n.id&&"(end)"!==n.id;){var a=u();">"===a.id?o--:"<"===a.id&&o++,i+=a.value}u(">");try{this.signature=e(i)}catch(e){throw e.position=r+e.offset,e}}u("{"),this.body=c(0),u("}")}return this}),l("(",function(){for(var e=[];")"!==n.id&&(e.push(c(0)),";"===n.id);)u(";");return u(")",!0),this.type="block",this.expressions=e,this}),l("[",function(){var e=[];if("]"!==n.id)for(;;){var t=c(0);if(".."===n.id){var r={type:"binary",value:"..",position:n.position,lhs:t};u(".."),r.rhs=c(0),t=r}if(e.push(t),","!==n.id)break;u(",")}return u("]",!0),this.lhs=e,this.type="unary",this}),f("[",me["["],function(e){if("]"===n.id){for(var t=e;t&&"binary"===t.type&&"["===t.value;)t=t.lhs;return t.keepArray=!0,u("]"),e}return this.lhs=e,this.rhs=c(me["]"]),this.type="binary",u("]",!0),this}),f("^",me["^"],function(e){u("(");for(var t=[];;){var r={descending:!1};if("<"===n.id?u("<"):">"===n.id&&(r.descending=!0,u(">")),r.expression=c(0),t.push(r),","!==n.id)break;u(",")}return u(")"),this.lhs=e,this.rhs=t,this.type="binary",this});var p=function(e){var t=[];if("}"!==n.id)for(;;){var r=c(0);u(":");var o=c(0);if(t.push([r,o]),","!==n.id)break;u(",")}return u("}",!0),void 0===e?(this.lhs=t,this.type="unary"):(this.lhs=e,this.rhs=t,this.type="binary"),this};l("{",p),f("{",me["{"],p),f("?",me["?"],function(e){return this.type="condition",this.condition=e,this.then=c(0),":"===n.id&&(u(":"),this.else=c(0)),this});var h=function e(t){var r;if("function"===t.type){var n={type:"lambda",thunk:!0,arguments:[],position:t.position};n.body=t,r=n}else if("condition"===t.type)t.then=e(t.then),void 0!==t.else&&(t.else=e(t.else)),r=t;else if("block"===t.type){var o=t.expressions.length;o>0&&(t.expressions[o-1]=e(t.expressions[o-1])),r=t}else r=t;return r};o=xe(r),u();var b=c(0);if("(end)"!==n.id)throw{code:"S0201",stack:(new Error).stack,position:n.position,token:n.value};return b=function e(r){var n;switch(r.type){case"binary":switch(r.value){case".":var o=e(r.lhs);n={type:"path",steps:[]},"path"===o.type?Array.prototype.push.apply(n.steps,o.steps):n.steps=[o];var i=e(r.rhs);"path"!==i.type&&(i={type:"path",steps:[i]}),Array.prototype.push.apply(n.steps,i.steps),n.steps.filter(function(e){return"literal"===e.type}).forEach(function(e){e.type="name"}),n.steps.filter(function(e){return!0===e.keepArray}).length>0&&(n.keepSingletonArray=!0),"unary"===n.steps[0].type&&"["===n.steps[0].value&&(n.steps[0].consarray=!0);break;case"[":var a=n=e(r.lhs);if("path"===n.type&&(a=n.steps[n.steps.length-1]),void 0!==a.group)throw{code:"S0209",stack:(new Error).stack,position:r.position};void 0===a.predicate&&(a.predicate=[]),a.predicate.push(e(r.rhs));break;case"{":if(void 0!==(n=e(r.lhs)).group)throw{code:"S0210",stack:(new Error).stack,position:r.position};n.group={lhs:r.rhs.map(function(t){return[e(t[0]),e(t[1])]}),position:r.position};break;case"^":(n={type:"sort",value:r.value,position:r.position}).lhs=e(r.lhs),n.rhs=r.rhs.map(function(t){return{descending:t.descending,expression:e(t.expression)}});break;case":=":(n={type:"bind",value:r.value,position:r.position}).lhs=e(r.lhs),n.rhs=e(r.rhs);break;case"~>":(n={type:"apply",value:r.value,position:r.position}).lhs=e(r.lhs),n.rhs=e(r.rhs);break;default:(n={type:r.type,value:r.value,position:r.position}).lhs=e(r.lhs),n.rhs=e(r.rhs)}break;case"unary":n={type:r.type,value:r.value,position:r.position},"["===r.value?n.lhs=r.lhs.map(function(t){return e(t)}):"{"===r.value?n.lhs=r.lhs.map(function(t){return[e(t[0]),e(t[1])]}):(n.expression=e(r.expression),"-"===r.value&&"literal"===n.expression.type&&t(n.expression.value)&&((n=n.expression).value=-n.value));break;case"function":case"partial":(n={type:r.type,name:r.name,value:r.value,position:r.position}).arguments=r.arguments.map(function(t){return e(t)}),n.procedure=e(r.procedure);break;case"lambda":n={type:r.type,arguments:r.arguments,signature:r.signature,position:r.position};var s=e(r.body);n.body=h(s);break;case"condition":(n={type:r.type,position:r.position}).condition=e(r.condition),n.then=e(r.then),void 0!==r.else&&(n.else=e(r.else));break;case"block":(n={type:r.type,position:r.position}).expressions=r.expressions.map(function(t){return e(t)});break;case"name":n={type:"path",steps:[r]},r.keepArray&&(n.keepSingletonArray=!0);break;case"literal":case"wildcard":case"descendant":case"variable":case"regex":n=r;break;case"operator":if("and"===r.value||"or"===r.value||"in"===r.value)r.type="name",n=e(r);else{if("?"!==r.value)throw{code:"S0201",stack:(new Error).stack,position:r.position,token:r.value};n=r}break;default:var u="S0206";throw"(end)"===r.id&&(u="S0207"),{code:u,stack:(new Error).stack,position:r.position,token:r.value}}return n}(b)},je=ve(null);Number.isInteger=p.default||function(e){return"number"==typeof e&&isFinite(e)&&Math.floor(e)===e};var Ee=G(we("function($f, $g) { function($x){ $g($f($x)) } }"),null,je);je.bind("sum",oe(function(e){if(void 0!==e){var t=0;return e.forEach(function(e){t+=e}),t}},"<a<n>:n>")),je.bind("count",oe(function(e){return void 0===e?0:e.length},"<a:n>")),je.bind("max",oe(function(e){if(void 0!==e&&0!==e.length)return Math.max.apply(Math,e)},"<a<n>:n>")),je.bind("min",oe(function(e){if(void 0!==e&&0!==e.length)return Math.min.apply(Math,e)},"<a<n>:n>")),je.bind("average",oe(function(e){if(void 0!==e&&0!==e.length){var t=0;return e.forEach(function(e){t+=e}),t/e.length}},"<a<n>:n>")),je.bind("string",oe(ie,"<x-:s>")),je.bind("substring",oe(function(e,t,r){if(void 0!==e)return e.substr(t,r)},"<s-nn?:s>")),je.bind("substringBefore",oe(function(e,t){if(void 0!==e){var r=e.indexOf(t);return r>-1?e.substr(0,r):e}},"<s-s:s>")),je.bind("substringAfter",oe(function(e,t){if(void 0!==e){var r=e.indexOf(t);return r>-1?e.substr(r+t.length):e}},"<s-s:s>")),je.bind("lowercase",oe(function(e){if(void 0!==e)return e.toLowerCase()},"<s-:s>")),je.bind("uppercase",oe(function(e){if(void 0!==e)return e.toUpperCase()},"<s-:s>")),je.bind("length",oe(function(e){if(void 0!==e)return e.length},"<s-:n>")),je.bind("trim",oe(function(e){if(void 0!==e){var t=e.replace(/[ \t\n\r]+/gm," ");return" "===t.charAt(0)&&(t=t.substring(1))," "===t.charAt(t.length-1)&&(t=t.substring(0,t.length-1)),t}},"<s-:s>")),je.bind("match",oe(function(e,t,r){if(void 0!==e){if(r<0)throw{stack:(new Error).stack,value:r,code:"D3040",index:3};var n=[];if(void 0===r||r>0){var o=0,i=t(e);if(void 0!==i)for(;void 0!==i&&(void 0===r||o<r);)n.push({match:i.match,index:i.start,groups:i.groups}),i=i.next(),o++}return n}},"<s-f<s:o>n?:a<o>>")),je.bind("contains",oe(function(e,t){if(void 0!==e)return"string"==typeof t?-1!==e.indexOf(t):void 0!==t(e)},"<s-(sf):b>")),je.bind("replace",oe(ae,"<s-(sf)(sf)n?:s>")),je.bind("split",oe(function(e,t,r){if(void 0!==e){if(r<0)throw{code:"D3020",stack:(new Error).stack,value:r,index:3};var n=[];if(void 0===r||r>0)if("string"==typeof t)n=e.split(t,r);else{var o=0,i=t(e);if(void 0!==i){for(var a=0;void 0!==i&&(void 0===r||o<r);)n.push(e.substring(a,i.start)),a=i.end,i=i.next(),o++;(void 0===r||o<r)&&n.push(e.substring(a))}else n=[e]}return n}},"<s-(sf)n?:a<s>>")),je.bind("join",oe(function(e,t){if(void 0!==e)return void 0===t&&(t=""),e.join(t)},"<a<s>s?:s>")),je.bind("number",oe(function(e){var t;if(void 0!==e){if("number"==typeof e)t=e;else{if("string"!=typeof e||!/^-?(0|([1-9][0-9]*))(\.[0-9]+)?([Ee][-+]?[0-9]+)?$/.test(e)||isNaN(parseFloat(e))||!isFinite(e))throw{code:"D3030",value:e,stack:(new Error).stack,index:1};t=parseFloat(e)}return t}},"<(ns)-:n>")),je.bind("floor",oe(function(e){if(void 0!==e)return Math.floor(e)},"<n-:n>")),je.bind("ceil",oe(function(e){if(void 0!==e)return Math.ceil(e)},"<n-:n>")),je.bind("round",oe(function(e,t){var r;if(void 0!==e){if(t){var n=e.toString().split("e");e=+(n[0]+"e"+(n[1]?+n[1]+t:t))}var o=(r=Math.round(e))-e;return.5===Math.abs(o)&&1===Math.abs(r%2)&&(r-=1),t&&(r=+((n=r.toString().split("e"))[0]+"e"+(n[1]?+n[1]-t:-t))),(0,i.default)(r,-0)&&(r=0),r}},"<n-n?:n>")),je.bind("abs",oe(function(e){if(void 0!==e)return Math.abs(e)},"<n-:n>")),je.bind("sqrt",oe(function(e){if(void 0!==e){if(e<0)throw{stack:(new Error).stack,code:"D3060",index:1,value:e};return Math.sqrt(e)}},"<n-:n>")),je.bind("power",oe(function(e,t){var r;if(void 0!==e){if(r=Math.pow(e,t),!isFinite(r))throw{stack:(new Error).stack,code:"D3061",index:1,value:e,exp:t};return r}},"<n-n:n>")),je.bind("random",oe(function(){return Math.random()},"<:n>")),je.bind("boolean",oe(se,"<x-:b>")),je.bind("not",oe(function(e){return!se(e)},"<x-:b>")),je.bind("map",oe(ue,"<af>")),je.bind("zip",oe(function(){for(var e=[],t=Array.prototype.slice.call(arguments),r=Math.min.apply(Math,t.map(function(e){return Array.isArray(e)?e.length:0})),n=0;n<r;n++){var o=t.map(function(e){return e[n]});e.push(o)}return e},"<a+>")),je.bind("filter",oe(ce,"<af>")),je.bind("reduce",oe(fe,"<afj?:j>")),je.bind("sift",oe(function(e,t){var r={};for(var n in e){var o=e[n];se(function(e,r,n){for(var o=z(t,[e,r,n],null),i=o.next();!i.done;)i=o.next(i.value);return i.value}(o,n,e))&&(r[n]=o)}return 0===(0,c.default)(r).length&&(r=void 0),r},"<o-f?:o>")),je.bind("keys",oe(le,"<x-:a<s>>")),je.bind("lookup",oe(function(e,t){return w({value:t},e)},"<x-s:x>")),je.bind("append",oe(pe,"<xx:a>")),je.bind("exists",oe(function(e){return void 0!==e},"<x:b>")),je.bind("spread",oe(de,"<x-:a<o>>")),je.bind("reverse",oe(function(e){if(void 0!==e){if(e.length<=1)return e;for(var t=e.length,r=new Array(t),n=0;n<t;n++)r[t-n-1]=e[n];return r}},"<a:a>")),je.bind("each",oe(he,"<o-f:a>")),je.bind("sort",oe(be,"<af?:a>")),je.bind("shuffle",oe(function(e){if(void 0!==e){if(e.length<=1)return e;for(var t=new Array(e.length),r=0;r<e.length;r++){var n=Math.floor(Math.random()*(r+1));r!==n&&(t[r]=t[n]),t[n]=e[r]}return t}},"<a:a>")),je.bind("base64encode",oe(function(e){if(void 0!==e)return("undefined"!=typeof window?window.btoa:function(e){return new r.Buffer(e,"binary").toString("base64")})(e)},"<s-:s>")),je.bind("base64decode",oe(function(e){if(void 0!==e)return("undefined"!=typeof window?window.atob:function(e){return new r.Buffer(e,"base64").toString("binary")})(e)},"<s-:s>"));var Se={S0101:"String literal must be terminated by a matching quote",S0102:"Number out of range: {{token}}",S0103:"Unsupported escape sequence: \\{{token}}",S0104:"The escape sequence \\u must be followed by 4 hex digits",S0203:"Expected {{value}} before end of expression",S0202:"Expected {{value}}, got {{token}}",S0204:"Unknown operator: {{token}}",S0205:"Unexpected token: {{token}}",S0208:"Parameter {{value}} of function definition must be a variable name (start with $)",S0209:"A predicate cannot follow a grouping expression in a step",S0210:"Each step can only have one grouping expression",S0201:"Syntax error: {{token}}",S0206:"Unknown expression type: {{token}}",S0207:"Unexpected end of expression",S0301:"Empty regular expressions are not allowed",S0302:"No terminating / in regular expression",S0402:"Choice groups containing parameterized types are not supported",S0401:"Type parameters can only be applied to functions and arrays",T0410:"Argument {{index}} of function {{token}} does not match function signature",T0411:"Context value is not a compatible type with argument {{index}} of function {{token}}",T0412:"Argument {{index}} of function {{token}} must be an array of {{type}}",D1001:"Number out of range: {{value}}",D1002:"Cannot negate a non-numeric value: {{value}}",T2001:"The left side of the {{token}} operator must evaluate to a number",T2002:"The right side of the {{token}} operator must evaluate to a number",T1003:"Key in object structure must evaluate to a string; got: {{value}}",T2003:"The left side of the range operator (..) must evaluate to an integer",T2004:"The right side of the range operator (..) must evaluate to an integer",D2005:"The left side of := must be a variable name (start with $)",D1004:"Regular expression matches zero length string",T2006:"The right side of the function application operator ~> must be a function",T2007:"Type mismatch when comparing values {{value}} and {{value2}} in order-by clause",T2008:"The expressions within an order-by clause must evaluate to numeric or string values",T2009:"The values {{value}} and {{value2}} either side of operator {{token}} must be of the same data type",T2010:"The expressions either side of operator {{token}} must evaluate to numeric or string values",T1005:"Attempted to invoke a non-function. Did you mean ${{{token}}}?",T1006:"Attempted to invoke a non-function",T1007:"Attempted to partially apply a non-function. Did you mean ${{{token}}}?",T1008:"Attempted to partially apply a non-function",D3001:"Attempting to invoke string function on Infinity or NaN",D3010:"Second argument of replace function cannot be an empty string",D3011:"Fourth argument of replace function must evaluate to a positive number",D3012:"Attempted to replace a matched string with a non-string value",D3020:"Third argument of split function must evaluate to a positive number",D3030:"Unable to cast value to a number: {{value}}",D3040:"Third argument of match function must evaluate to a positive number",D3050:"First argument of reduce function must be a function with two arguments",D3060:"The sqrt function cannot be applied to a negative number: {{value}}",D3061:"The power function has resulted in a value that cannot be represented as a JSON number: base={{value}}, exponent={{exp}}",D3070:"The single argument form of the sort function can only be applied to an array of strings or an array of numbers.  Use the second argument to specify a comparison function"};return _e.parser=we,_e}();void 0!==t&&(t.exports=b)}).call(this,"undefined"!=typeof global?global:"undefined"!=typeof self?self:"undefined"!=typeof window?window:{})},{"babel-runtime/core-js/is-iterable":2,"babel-runtime/core-js/json/stringify":3,"babel-runtime/core-js/number/is-integer":4,"babel-runtime/core-js/object/create":5,"babel-runtime/core-js/object/is":6,"babel-runtime/core-js/object/keys":7,"babel-runtime/core-js/promise":8,"babel-runtime/core-js/symbol":9,"babel-runtime/core-js/symbol/iterator":10,"babel-runtime/regenerator":11}],2:[function(e,t,r){t.exports={default:e("core-js/library/fn/is-iterable"),__esModule:!0}},{"core-js/library/fn/is-iterable":12}],3:[function(e,t,r){t.exports={default:e("core-js/library/fn/json/stringify"),__esModule:!0}},{"core-js/library/fn/json/stringify":13}],4:[function(e,t,r){t.exports={default:e("core-js/library/fn/number/is-integer"),__esModule:!0}},{"core-js/library/fn/number/is-integer":14}],5:[function(e,t,r){t.exports={default:e("core-js/library/fn/object/create"),__esModule:!0}},{"core-js/library/fn/object/create":15}],6:[function(e,t,r){t.exports={default:e("core-js/library/fn/object/is"),__esModule:!0}},{"core-js/library/fn/object/is":16}],7:[function(e,t,r){t.exports={default:e("core-js/library/fn/object/keys"),__esModule:!0}},{"core-js/library/fn/object/keys":17}],8:[function(e,t,r){t.exports={default:e("core-js/library/fn/promise"),__esModule:!0}},{"core-js/library/fn/promise":18}],9:[function(e,t,r){t.exports={default:e("core-js/library/fn/symbol"),__esModule:!0}},{"core-js/library/fn/symbol":19}],10:[function(e,t,r){t.exports={default:e("core-js/library/fn/symbol/iterator"),__esModule:!0}},{"core-js/library/fn/symbol/iterator":20}],11:[function(e,t,r){t.exports=e("regenerator-runtime")},{"regenerator-runtime":106}],12:[function(e,t,r){e("../modules/web.dom.iterable"),e("../modules/es6.string.iterator"),t.exports=e("../modules/core.is-iterable")},{"../modules/core.is-iterable":93,"../modules/es6.string.iterator":101,"../modules/web.dom.iterable":105}],13:[function(e,t,r){var n=e("../../modules/_core"),o=n.JSON||(n.JSON={stringify:JSON.stringify});t.exports=function(e){return o.stringify.apply(o,arguments)}},{"../../modules/_core":28}],14:[function(e,t,r){e("../../modules/es6.number.is-integer"),t.exports=e("../../modules/_core").Number.isInteger},{"../../modules/_core":28,"../../modules/es6.number.is-integer":95}],15:[function(e,t,r){e("../../modules/es6.object.create");var n=e("../../modules/_core").Object;t.exports=function(e,t){return n.create(e,t)}},{"../../modules/_core":28,"../../modules/es6.object.create":96}],16:[function(e,t,r){e("../../modules/es6.object.is"),t.exports=e("../../modules/_core").Object.is},{"../../modules/_core":28,"../../modules/es6.object.is":97}],17:[function(e,t,r){e("../../modules/es6.object.keys"),t.exports=e("../../modules/_core").Object.keys},{"../../modules/_core":28,"../../modules/es6.object.keys":98}],18:[function(e,t,r){e("../modules/es6.object.to-string"),e("../modules/es6.string.iterator"),e("../modules/web.dom.iterable"),e("../modules/es6.promise"),t.exports=e("../modules/_core").Promise},{"../modules/_core":28,"../modules/es6.object.to-string":99,"../modules/es6.promise":100,"../modules/es6.string.iterator":101,"../modules/web.dom.iterable":105}],19:[function(e,t,r){e("../../modules/es6.symbol"),e("../../modules/es6.object.to-string"),e("../../modules/es7.symbol.async-iterator"),e("../../modules/es7.symbol.observable"),t.exports=e("../../modules/_core").Symbol},{"../../modules/_core":28,"../../modules/es6.object.to-string":99,"../../modules/es6.symbol":102,"../../modules/es7.symbol.async-iterator":103,"../../modules/es7.symbol.observable":104}],20:[function(e,t,r){e("../../modules/es6.string.iterator"),e("../../modules/web.dom.iterable"),t.exports=e("../../modules/_wks-ext").f("iterator")},{"../../modules/_wks-ext":90,"../../modules/es6.string.iterator":101,"../../modules/web.dom.iterable":105}],21:[function(e,t,r){t.exports=function(e){if("function"!=typeof e)throw TypeError(e+" is not a function!");return e}},{}],22:[function(e,t,r){t.exports=function(){}},{}],23:[function(e,t,r){t.exports=function(e,t,r,n){if(!(e instanceof t)||void 0!==n&&n in e)throw TypeError(r+": incorrect invocation!");return e}},{}],24:[function(e,t,r){var n=e("./_is-object");t.exports=function(e){if(!n(e))throw TypeError(e+" is not an object!");return e}},{"./_is-object":48}],25:[function(e,t,r){var n=e("./_to-iobject"),o=e("./_to-length"),i=e("./_to-index");t.exports=function(e){return function(t,r,a){var s,u=n(t),c=o(u.length),f=i(a,c);if(e&&r!=r){for(;c>f;)if((s=u[f++])!=s)return!0}else for(;c>f;f++)if((e||f in u)&&u[f]===r)return e||f||0;return!e&&-1}}},{"./_to-index":82,"./_to-iobject":84,"./_to-length":85}],26:[function(e,t,r){var n=e("./_cof"),o=e("./_wks")("toStringTag"),i="Arguments"==n(function(){return arguments}()),a=function(e,t){try{return e[t]}catch(e){}};t.exports=function(e){var t,r,s;return void 0===e?"Undefined":null===e?"Null":"string"==typeof(r=a(t=Object(e),o))?r:i?n(t):"Object"==(s=n(t))&&"function"==typeof t.callee?"Arguments":s}},{"./_cof":27,"./_wks":91}],27:[function(e,t,r){var n={}.toString;t.exports=function(e){return n.call(e).slice(8,-1)}},{}],28:[function(e,t,r){var n=t.exports={version:"2.4.0"};"number"==typeof __e&&(__e=n)},{}],29:[function(e,t,r){var n=e("./_a-function");t.exports=function(e,t,r){if(n(e),void 0===t)return e;switch(r){case 1:return function(r){return e.call(t,r)};case 2:return function(r,n){return e.call(t,r,n)};case 3:return function(r,n,o){return e.call(t,r,n,o)}}return function(){return e.apply(t,arguments)}}},{"./_a-function":21}],30:[function(e,t,r){t.exports=function(e){if(void 0==e)throw TypeError("Can't call method on  "+e);return e}},{}],31:[function(e,t,r){t.exports=!e("./_fails")(function(){return 7!=Object.defineProperty({},"a",{get:function(){return 7}}).a})},{"./_fails":36}],32:[function(e,t,r){var n=e("./_is-object"),o=e("./_global").document,i=n(o)&&n(o.createElement);t.exports=function(e){return i?o.createElement(e):{}}},{"./_global":38,"./_is-object":48}],33:[function(e,t,r){t.exports="constructor,hasOwnProperty,isPrototypeOf,propertyIsEnumerable,toLocaleString,toString,valueOf".split(",")},{}],34:[function(e,t,r){var n=e("./_object-keys"),o=e("./_object-gops"),i=e("./_object-pie");t.exports=function(e){var t=n(e),r=o.f;if(r)for(var a,s=r(e),u=i.f,c=0;s.length>c;)u.call(e,a=s[c++])&&t.push(a);return t}},{"./_object-gops":65,"./_object-keys":68,"./_object-pie":69}],35:[function(e,t,r){var n=e("./_global"),o=e("./_core"),i=e("./_ctx"),a=e("./_hide"),s=function(e,t,r){var u,c,f,l=e&s.F,p=e&s.G,d=e&s.S,h=e&s.P,b=e&s.B,v=e&s.W,y=p?o:o[t]||(o[t]={}),_=y.prototype,g=p?n:d?n[t]:(n[t]||{}).prototype;p&&(r=t);for(u in r)(c=!l&&g&&void 0!==g[u])&&u in y||(f=c?g[u]:r[u],y[u]=p&&"function"!=typeof g[u]?r[u]:b&&c?i(f,n):v&&g[u]==f?function(e){var t=function(t,r,n){if(this instanceof e){switch(arguments.length){case 0:return new e;case 1:return new e(t);case 2:return new e(t,r)}return new e(t,r,n)}return e.apply(this,arguments)};return t.prototype=e.prototype,t}(f):h&&"function"==typeof f?i(Function.call,f):f,h&&((y.virtual||(y.virtual={}))[u]=f,e&s.R&&_&&!_[u]&&a(_,u,f)))};s.F=1,s.G=2,s.S=4,s.P=8,s.B=16,s.W=32,s.U=64,s.R=128,t.exports=s},{"./_core":28,"./_ctx":29,"./_global":38,"./_hide":40}],36:[function(e,t,r){t.exports=function(e){try{return!!e()}catch(e){return!0}}},{}],37:[function(e,t,r){var n=e("./_ctx"),o=e("./_iter-call"),i=e("./_is-array-iter"),a=e("./_an-object"),s=e("./_to-length"),u=e("./core.get-iterator-method"),c={},f={};(r=t.exports=function(e,t,r,l,p){var d,h,b,v,y=p?function(){return e}:u(e),_=n(r,l,t?2:1),g=0;if("function"!=typeof y)throw TypeError(e+" is not iterable!");if(i(y)){for(d=s(e.length);d>g;g++)if((v=t?_(a(h=e[g])[0],h[1]):_(e[g]))===c||v===f)return v}else for(b=y.call(e);!(h=b.next()).done;)if((v=o(b,_,h.value,t))===c||v===f)return v}).BREAK=c,r.RETURN=f},{"./_an-object":24,"./_ctx":29,"./_is-array-iter":45,"./_iter-call":49,"./_to-length":85,"./core.get-iterator-method":92}],38:[function(e,t,r){var n=t.exports="undefined"!=typeof window&&window.Math==Math?window:"undefined"!=typeof self&&self.Math==Math?self:Function("return this")();"number"==typeof __g&&(__g=n)},{}],39:[function(e,t,r){var n={}.hasOwnProperty;t.exports=function(e,t){return n.call(e,t)}},{}],40:[function(e,t,r){var n=e("./_object-dp"),o=e("./_property-desc");t.exports=e("./_descriptors")?function(e,t,r){return n.f(e,t,o(1,r))}:function(e,t,r){return e[t]=r,e}},{"./_descriptors":31,"./_object-dp":60,"./_property-desc":71}],41:[function(e,t,r){t.exports=e("./_global").document&&document.documentElement},{"./_global":38}],42:[function(e,t,r){t.exports=!e("./_descriptors")&&!e("./_fails")(function(){return 7!=Object.defineProperty(e("./_dom-create")("div"),"a",{get:function(){return 7}}).a})},{"./_descriptors":31,"./_dom-create":32,"./_fails":36}],43:[function(e,t,r){t.exports=function(e,t,r){var n=void 0===r;switch(t.length){case 0:return n?e():e.call(r);case 1:return n?e(t[0]):e.call(r,t[0]);case 2:return n?e(t[0],t[1]):e.call(r,t[0],t[1]);case 3:return n?e(t[0],t[1],t[2]):e.call(r,t[0],t[1],t[2]);case 4:return n?e(t[0],t[1],t[2],t[3]):e.call(r,t[0],t[1],t[2],t[3])}return e.apply(r,t)}},{}],44:[function(e,t,r){var n=e("./_cof");t.exports=Object("z").propertyIsEnumerable(0)?Object:function(e){return"String"==n(e)?e.split(""):Object(e)}},{"./_cof":27}],45:[function(e,t,r){var n=e("./_iterators"),o=e("./_wks")("iterator"),i=Array.prototype;t.exports=function(e){return void 0!==e&&(n.Array===e||i[o]===e)}},{"./_iterators":54,"./_wks":91}],46:[function(e,t,r){var n=e("./_cof");t.exports=Array.isArray||function(e){return"Array"==n(e)}},{"./_cof":27}],47:[function(e,t,r){var n=e("./_is-object"),o=Math.floor;t.exports=function(e){return!n(e)&&isFinite(e)&&o(e)===e}},{"./_is-object":48}],48:[function(e,t,r){t.exports=function(e){return"object"==typeof e?null!==e:"function"==typeof e}},{}],49:[function(e,t,r){var n=e("./_an-object");t.exports=function(e,t,r,o){try{return o?t(n(r)[0],r[1]):t(r)}catch(t){var i=e.return;throw void 0!==i&&n(i.call(e)),t}}},{"./_an-object":24}],50:[function(e,t,r){"use strict";var n=e("./_object-create"),o=e("./_property-desc"),i=e("./_set-to-string-tag"),a={};e("./_hide")(a,e("./_wks")("iterator"),function(){return this}),t.exports=function(e,t,r){e.prototype=n(a,{next:o(1,r)}),i(e,t+" Iterator")}},{"./_hide":40,"./_object-create":59,"./_property-desc":71,"./_set-to-string-tag":76,"./_wks":91}],51:[function(e,t,r){"use strict";var n=e("./_library"),o=e("./_export"),i=e("./_redefine"),a=e("./_hide"),s=e("./_has"),u=e("./_iterators"),c=e("./_iter-create"),f=e("./_set-to-string-tag"),l=e("./_object-gpo"),p=e("./_wks")("iterator"),d=!([].keys&&"next"in[].keys()),h=function(){return this};t.exports=function(e,t,r,b,v,y,_){c(r,t,b);var g,m,k,x=function(e){if(!d&&e in S)return S[e];switch(e){case"keys":case"values":return function(){return new r(this,e)}}return function(){return new r(this,e)}},w=t+" Iterator",j="values"==v,E=!1,S=e.prototype,A=S[p]||S["@@iterator"]||v&&S[v],O=A||x(v),T=v?j?x("entries"):O:void 0,P="Array"==t?S.entries||A:A;if(P&&(k=l(P.call(new e)))!==Object.prototype&&(f(k,w,!0),n||s(k,p)||a(k,p,h)),j&&A&&"values"!==A.name&&(E=!0,O=function(){return A.call(this)}),n&&!_||!d&&!E&&S[p]||a(S,p,O),u[t]=O,u[w]=h,v)if(g={values:j?O:x("values"),keys:y?O:x("keys"),entries:T},_)for(m in g)m in S||i(S,m,g[m]);else o(o.P+o.F*(d||E),t,g);return g}},{"./_export":35,"./_has":39,"./_hide":40,"./_iter-create":50,"./_iterators":54,"./_library":56,"./_object-gpo":66,"./_redefine":73,"./_set-to-string-tag":76,"./_wks":91}],52:[function(e,t,r){var n=e("./_wks")("iterator"),o=!1;try{var i=[7][n]();i.return=function(){o=!0},Array.from(i,function(){throw 2})}catch(e){}t.exports=function(e,t){if(!t&&!o)return!1;var r=!1;try{var i=[7],a=i[n]();a.next=function(){return{done:r=!0}},i[n]=function(){return a},e(i)}catch(e){}return r}},{"./_wks":91}],53:[function(e,t,r){t.exports=function(e,t){return{value:t,done:!!e}}},{}],54:[function(e,t,r){t.exports={}},{}],55:[function(e,t,r){var n=e("./_object-keys"),o=e("./_to-iobject");t.exports=function(e,t){for(var r,i=o(e),a=n(i),s=a.length,u=0;s>u;)if(i[r=a[u++]]===t)return r}},{"./_object-keys":68,"./_to-iobject":84}],56:[function(e,t,r){t.exports=!0},{}],57:[function(e,t,r){var n=e("./_uid")("meta"),o=e("./_is-object"),i=e("./_has"),a=e("./_object-dp").f,s=0,u=Object.isExtensible||function(){return!0},c=!e("./_fails")(function(){return u(Object.preventExtensions({}))}),f=function(e){a(e,n,{value:{i:"O"+ ++s,w:{}}})},l=t.exports={KEY:n,NEED:!1,fastKey:function(e,t){if(!o(e))return"symbol"==typeof e?e:("string"==typeof e?"S":"P")+e;if(!i(e,n)){if(!u(e))return"F";if(!t)return"E";f(e)}return e[n].i},getWeak:function(e,t){if(!i(e,n)){if(!u(e))return!0;if(!t)return!1;f(e)}return e[n].w},onFreeze:function(e){return c&&l.NEED&&u(e)&&!i(e,n)&&f(e),e}}},{"./_fails":36,"./_has":39,"./_is-object":48,"./_object-dp":60,"./_uid":88}],58:[function(e,t,r){var n=e("./_global"),o=e("./_task").set,i=n.MutationObserver||n.WebKitMutationObserver,a=n.process,s=n.Promise,u="process"==e("./_cof")(a);t.exports=function(){var e,t,r,c=function(){var n,o;for(u&&(n=a.domain)&&n.exit();e;){o=e.fn,e=e.next;try{o()}catch(n){throw e?r():t=void 0,n}}t=void 0,n&&n.enter()};if(u)r=function(){a.nextTick(c)};else if(i){var f=!0,l=document.createTextNode("");new i(c).observe(l,{characterData:!0}),r=function(){l.data=f=!f}}else if(s&&s.resolve){var p=s.resolve();r=function(){p.then(c)}}else r=function(){o.call(n,c)};return function(n){var o={fn:n,next:void 0};t&&(t.next=o),e||(e=o,r()),t=o}}},{"./_cof":27,"./_global":38,"./_task":81}],59:[function(e,t,r){var n=e("./_an-object"),o=e("./_object-dps"),i=e("./_enum-bug-keys"),a=e("./_shared-key")("IE_PROTO"),s=function(){},u=function(){var t,r=e("./_dom-create")("iframe"),n=i.length;for(r.style.display="none",e("./_html").appendChild(r),r.src="javascript:",(t=r.contentWindow.document).open(),t.write("<script>document.F=Object<\/script>"),t.close(),u=t.F;n--;)delete u.prototype[i[n]];return u()};t.exports=Object.create||function(e,t){var r;return null!==e?(s.prototype=n(e),r=new s,s.prototype=null,r[a]=e):r=u(),void 0===t?r:o(r,t)}},{"./_an-object":24,"./_dom-create":32,"./_enum-bug-keys":33,"./_html":41,"./_object-dps":61,"./_shared-key":77}],60:[function(e,t,r){var n=e("./_an-object"),o=e("./_ie8-dom-define"),i=e("./_to-primitive"),a=Object.defineProperty;r.f=e("./_descriptors")?Object.defineProperty:function(e,t,r){if(n(e),t=i(t,!0),n(r),o)try{return a(e,t,r)}catch(e){}if("get"in r||"set"in r)throw TypeError("Accessors not supported!");return"value"in r&&(e[t]=r.value),e}},{"./_an-object":24,"./_descriptors":31,"./_ie8-dom-define":42,"./_to-primitive":87}],61:[function(e,t,r){var n=e("./_object-dp"),o=e("./_an-object"),i=e("./_object-keys");t.exports=e("./_descriptors")?Object.defineProperties:function(e,t){o(e);for(var r,a=i(t),s=a.length,u=0;s>u;)n.f(e,r=a[u++],t[r]);return e}},{"./_an-object":24,"./_descriptors":31,"./_object-dp":60,"./_object-keys":68}],62:[function(e,t,r){var n=e("./_object-pie"),o=e("./_property-desc"),i=e("./_to-iobject"),a=e("./_to-primitive"),s=e("./_has"),u=e("./_ie8-dom-define"),c=Object.getOwnPropertyDescriptor;r.f=e("./_descriptors")?c:function(e,t){if(e=i(e),t=a(t,!0),u)try{return c(e,t)}catch(e){}if(s(e,t))return o(!n.f.call(e,t),e[t])}},{"./_descriptors":31,"./_has":39,"./_ie8-dom-define":42,"./_object-pie":69,"./_property-desc":71,"./_to-iobject":84,"./_to-primitive":87}],63:[function(e,t,r){var n=e("./_to-iobject"),o=e("./_object-gopn").f,i={}.toString,a="object"==typeof window&&window&&Object.getOwnPropertyNames?Object.getOwnPropertyNames(window):[],s=function(e){try{return o(e)}catch(e){return a.slice()}};t.exports.f=function(e){return a&&"[object Window]"==i.call(e)?s(e):o(n(e))}},{"./_object-gopn":64,"./_to-iobject":84}],64:[function(e,t,r){var n=e("./_object-keys-internal"),o=e("./_enum-bug-keys").concat("length","prototype");r.f=Object.getOwnPropertyNames||function(e){return n(e,o)}},{"./_enum-bug-keys":33,"./_object-keys-internal":67}],65:[function(e,t,r){r.f=Object.getOwnPropertySymbols},{}],66:[function(e,t,r){var n=e("./_has"),o=e("./_to-object"),i=e("./_shared-key")("IE_PROTO"),a=Object.prototype;t.exports=Object.getPrototypeOf||function(e){return e=o(e),n(e,i)?e[i]:"function"==typeof e.constructor&&e instanceof e.constructor?e.constructor.prototype:e instanceof Object?a:null}},{"./_has":39,"./_shared-key":77,"./_to-object":86}],67:[function(e,t,r){var n=e("./_has"),o=e("./_to-iobject"),i=e("./_array-includes")(!1),a=e("./_shared-key")("IE_PROTO");t.exports=function(e,t){var r,s=o(e),u=0,c=[];for(r in s)r!=a&&n(s,r)&&c.push(r);for(;t.length>u;)n(s,r=t[u++])&&(~i(c,r)||c.push(r));return c}},{"./_array-includes":25,"./_has":39,"./_shared-key":77,"./_to-iobject":84}],68:[function(e,t,r){var n=e("./_object-keys-internal"),o=e("./_enum-bug-keys");t.exports=Object.keys||function(e){return n(e,o)}},{"./_enum-bug-keys":33,"./_object-keys-internal":67}],69:[function(e,t,r){r.f={}.propertyIsEnumerable},{}],70:[function(e,t,r){var n=e("./_export"),o=e("./_core"),i=e("./_fails");t.exports=function(e,t){var r=(o.Object||{})[e]||Object[e],a={};a[e]=t(r),n(n.S+n.F*i(function(){r(1)}),"Object",a)}},{"./_core":28,"./_export":35,"./_fails":36}],71:[function(e,t,r){t.exports=function(e,t){return{enumerable:!(1&e),configurable:!(2&e),writable:!(4&e),value:t}}},{}],72:[function(e,t,r){var n=e("./_hide");t.exports=function(e,t,r){for(var o in t)r&&e[o]?e[o]=t[o]:n(e,o,t[o]);return e}},{"./_hide":40}],73:[function(e,t,r){t.exports=e("./_hide")},{"./_hide":40}],74:[function(e,t,r){t.exports=Object.is||function(e,t){return e===t?0!==e||1/e==1/t:e!=e&&t!=t}},{}],75:[function(e,t,r){"use strict";var n=e("./_global"),o=e("./_core"),i=e("./_object-dp"),a=e("./_descriptors"),s=e("./_wks")("species");t.exports=function(e){var t="function"==typeof o[e]?o[e]:n[e];a&&t&&!t[s]&&i.f(t,s,{configurable:!0,get:function(){return this}})}},{"./_core":28,"./_descriptors":31,"./_global":38,"./_object-dp":60,"./_wks":91}],76:[function(e,t,r){var n=e("./_object-dp").f,o=e("./_has"),i=e("./_wks")("toStringTag");t.exports=function(e,t,r){e&&!o(e=r?e:e.prototype,i)&&n(e,i,{configurable:!0,value:t})}},{"./_has":39,"./_object-dp":60,"./_wks":91}],77:[function(e,t,r){var n=e("./_shared")("keys"),o=e("./_uid");t.exports=function(e){return n[e]||(n[e]=o(e))}},{"./_shared":78,"./_uid":88}],78:[function(e,t,r){var n=e("./_global"),o=n["__core-js_shared__"]||(n["__core-js_shared__"]={});t.exports=function(e){return o[e]||(o[e]={})}},{"./_global":38}],79:[function(e,t,r){var n=e("./_an-object"),o=e("./_a-function"),i=e("./_wks")("species");t.exports=function(e,t){var r,a=n(e).constructor;return void 0===a||void 0==(r=n(a)[i])?t:o(r)}},{"./_a-function":21,"./_an-object":24,"./_wks":91}],80:[function(e,t,r){var n=e("./_to-integer"),o=e("./_defined");t.exports=function(e){return function(t,r){var i,a,s=String(o(t)),u=n(r),c=s.length;return u<0||u>=c?e?"":void 0:(i=s.charCodeAt(u))<55296||i>56319||u+1===c||(a=s.charCodeAt(u+1))<56320||a>57343?e?s.charAt(u):i:e?s.slice(u,u+2):a-56320+(i-55296<<10)+65536}}},{"./_defined":30,"./_to-integer":83}],81:[function(e,t,r){var n,o,i,a=e("./_ctx"),s=e("./_invoke"),u=e("./_html"),c=e("./_dom-create"),f=e("./_global"),l=f.process,p=f.setImmediate,d=f.clearImmediate,h=f.MessageChannel,b=0,v={},y=function(){var e=+this;if(v.hasOwnProperty(e)){var t=v[e];delete v[e],t()}},_=function(e){y.call(e.data)};p&&d||(p=function(e){for(var t=[],r=1;arguments.length>r;)t.push(arguments[r++]);return v[++b]=function(){s("function"==typeof e?e:Function(e),t)},n(b),b},d=function(e){delete v[e]},"process"==e("./_cof")(l)?n=function(e){l.nextTick(a(y,e,1))}:h?(i=(o=new h).port2,o.port1.onmessage=_,n=a(i.postMessage,i,1)):f.addEventListener&&"function"==typeof postMessage&&!f.importScripts?(n=function(e){f.postMessage(e+"","*")},f.addEventListener("message",_,!1)):n="onreadystatechange"in c("script")?function(e){u.appendChild(c("script")).onreadystatechange=function(){u.removeChild(this),y.call(e)}}:function(e){setTimeout(a(y,e,1),0)}),t.exports={set:p,clear:d}},{"./_cof":27,"./_ctx":29,"./_dom-create":32,"./_global":38,"./_html":41,"./_invoke":43}],82:[function(e,t,r){var n=e("./_to-integer"),o=Math.max,i=Math.min;t.exports=function(e,t){return(e=n(e))<0?o(e+t,0):i(e,t)}},{"./_to-integer":83}],83:[function(e,t,r){var n=Math.ceil,o=Math.floor;t.exports=function(e){return isNaN(e=+e)?0:(e>0?o:n)(e)}},{}],84:[function(e,t,r){var n=e("./_iobject"),o=e("./_defined");t.exports=function(e){return n(o(e))}},{"./_defined":30,"./_iobject":44}],85:[function(e,t,r){var n=e("./_to-integer"),o=Math.min;t.exports=function(e){return e>0?o(n(e),9007199254740991):0}},{"./_to-integer":83}],86:[function(e,t,r){var n=e("./_defined");t.exports=function(e){return Object(n(e))}},{"./_defined":30}],87:[function(e,t,r){var n=e("./_is-object");t.exports=function(e,t){if(!n(e))return e;var r,o;if(t&&"function"==typeof(r=e.toString)&&!n(o=r.call(e)))return o;if("function"==typeof(r=e.valueOf)&&!n(o=r.call(e)))return o;if(!t&&"function"==typeof(r=e.toString)&&!n(o=r.call(e)))return o;throw TypeError("Can't convert object to primitive value")}},{"./_is-object":48}],88:[function(e,t,r){var n=0,o=Math.random();t.exports=function(e){return"Symbol(".concat(void 0===e?"":e,")_",(++n+o).toString(36))}},{}],89:[function(e,t,r){var n=e("./_global"),o=e("./_core"),i=e("./_library"),a=e("./_wks-ext"),s=e("./_object-dp").f;t.exports=function(e){var t=o.Symbol||(o.Symbol=i?{}:n.Symbol||{});"_"==e.charAt(0)||e in t||s(t,e,{value:a.f(e)})}},{"./_core":28,"./_global":38,"./_library":56,"./_object-dp":60,"./_wks-ext":90}],90:[function(e,t,r){r.f=e("./_wks")},{"./_wks":91}],91:[function(e,t,r){var n=e("./_shared")("wks"),o=e("./_uid"),i=e("./_global").Symbol,a="function"==typeof i;(t.exports=function(e){return n[e]||(n[e]=a&&i[e]||(a?i:o)("Symbol."+e))}).store=n},{"./_global":38,"./_shared":78,"./_uid":88}],92:[function(e,t,r){var n=e("./_classof"),o=e("./_wks")("iterator"),i=e("./_iterators");t.exports=e("./_core").getIteratorMethod=function(e){if(void 0!=e)return e[o]||e["@@iterator"]||i[n(e)]}},{"./_classof":26,"./_core":28,"./_iterators":54,"./_wks":91}],93:[function(e,t,r){var n=e("./_classof"),o=e("./_wks")("iterator"),i=e("./_iterators");t.exports=e("./_core").isIterable=function(e){var t=Object(e);return void 0!==t[o]||"@@iterator"in t||i.hasOwnProperty(n(t))}},{"./_classof":26,"./_core":28,"./_iterators":54,"./_wks":91}],94:[function(e,t,r){"use strict";var n=e("./_add-to-unscopables"),o=e("./_iter-step"),i=e("./_iterators"),a=e("./_to-iobject");t.exports=e("./_iter-define")(Array,"Array",function(e,t){this._t=a(e),this._i=0,this._k=t},function(){var e=this._t,t=this._k,r=this._i++;return!e||r>=e.length?(this._t=void 0,o(1)):"keys"==t?o(0,r):"values"==t?o(0,e[r]):o(0,[r,e[r]])},"values"),i.Arguments=i.Array,n("keys"),n("values"),n("entries")},{"./_add-to-unscopables":22,"./_iter-define":51,"./_iter-step":53,"./_iterators":54,"./_to-iobject":84}],95:[function(e,t,r){var n=e("./_export");n(n.S,"Number",{isInteger:e("./_is-integer")})},{"./_export":35,"./_is-integer":47}],96:[function(e,t,r){var n=e("./_export");n(n.S,"Object",{create:e("./_object-create")})},{"./_export":35,"./_object-create":59}],97:[function(e,t,r){var n=e("./_export");n(n.S,"Object",{is:e("./_same-value")})},{"./_export":35,"./_same-value":74}],98:[function(e,t,r){var n=e("./_to-object"),o=e("./_object-keys");e("./_object-sap")("keys",function(){return function(e){return o(n(e))}})},{"./_object-keys":68,"./_object-sap":70,"./_to-object":86}],99:[function(e,t,r){},{}],100:[function(e,t,r){"use strict";var n,o,i,a=e("./_library"),s=e("./_global"),u=e("./_ctx"),c=e("./_classof"),f=e("./_export"),l=e("./_is-object"),p=e("./_a-function"),d=e("./_an-instance"),h=e("./_for-of"),b=e("./_species-constructor"),v=e("./_task").set,y=e("./_microtask")(),_=s.TypeError,g=s.process,m=s.Promise,k="process"==c(g=s.process),x=function(){},w=!!function(){try{var t=m.resolve(1),r=(t.constructor={})[e("./_wks")("species")]=function(e){e(x,x)};return(k||"function"==typeof PromiseRejectionEvent)&&t.then(x)instanceof r}catch(e){}}(),j=function(e,t){return e===t||e===m&&t===i},E=function(e){var t;return!(!l(e)||"function"!=typeof(t=e.then))&&t},S=function(e){return j(m,e)?new A(e):new o(e)},A=o=function(e){var t,r;this.promise=new e(function(e,n){if(void 0!==t||void 0!==r)throw _("Bad Promise constructor");t=e,r=n}),this.resolve=p(t),this.reject=p(r)},O=function(e){try{e()}catch(e){return{error:e}}},T=function(e,t){if(!e._n){e._n=!0;var r=e._c;y(function(){for(var n=e._v,o=1==e._s,i=0;r.length>i;)!function(t){var r,i,a=o?t.ok:t.fail,s=t.resolve,u=t.reject,c=t.domain;try{a?(o||(2==e._h&&Y(e),e._h=1),!0===a?r=n:(c&&c.enter(),r=a(n),c&&c.exit()),r===t.promise?u(_("Promise-chain cycle")):(i=E(r))?i.call(r,s,u):s(r)):u(n)}catch(e){u(e)}}(r[i++]);e._c=[],e._n=!1,t&&!e._h&&P(e)})}},P=function(e){v.call(s,function(){var t,r,n,o=e._v;if(M(e)&&(t=O(function(){k?g.emit("unhandledRejection",o,e):(r=s.onunhandledrejection)?r({promise:e,reason:o}):(n=s.console)&&n.error&&n.error("Unhandled promise rejection",o)}),e._h=k||M(e)?2:1),e._a=void 0,t)throw t.error})},M=function(e){if(1==e._h)return!1;for(var t,r=e._a||e._c,n=0;r.length>n;)if((t=r[n++]).fail||!M(t.promise))return!1;return!0},Y=function(e){v.call(s,function(){var t;k?g.emit("rejectionHandled",e):(t=s.onrejectionhandled)&&t({promise:e,reason:e._v})})},N=function(e){var t=this;t._d||(t._d=!0,(t=t._w||t)._v=e,t._s=2,t._a||(t._a=t._c.slice()),T(t,!0))},D=function(e){var t,r=this;if(!r._d){r._d=!0,r=r._w||r;try{if(r===e)throw _("Promise can't be resolved itself");(t=E(e))?y(function(){var n={_w:r,_d:!1};try{t.call(e,u(D,n,1),u(N,n,1))}catch(e){N.call(n,e)}}):(r._v=e,r._s=1,T(r,!1))}catch(e){N.call({_w:r,_d:!1},e)}}};w||(m=function(e){d(this,m,"Promise","_h"),p(e),n.call(this);try{e(u(D,this,1),u(N,this,1))}catch(e){N.call(this,e)}},(n=function(e){this._c=[],this._a=void 0,this._s=0,this._d=!1,this._v=void 0,this._h=0,this._n=!1}).prototype=e("./_redefine-all")(m.prototype,{then:function(e,t){var r=S(b(this,m));return r.ok="function"!=typeof e||e,r.fail="function"==typeof t&&t,r.domain=k?g.domain:void 0,this._c.push(r),this._a&&this._a.push(r),this._s&&T(this,!1),r.promise},catch:function(e){return this.then(void 0,e)}}),A=function(){var e=new n;this.promise=e,this.resolve=u(D,e,1),this.reject=u(N,e,1)}),f(f.G+f.W+f.F*!w,{Promise:m}),e("./_set-to-string-tag")(m,"Promise"),e("./_set-species")("Promise"),i=e("./_core").Promise,f(f.S+f.F*!w,"Promise",{reject:function(e){var t=S(this);return(0,t.reject)(e),t.promise}}),f(f.S+f.F*(a||!w),"Promise",{resolve:function(e){if(e instanceof m&&j(e.constructor,this))return e;var t=S(this);return(0,t.resolve)(e),t.promise}}),f(f.S+f.F*!(w&&e("./_iter-detect")(function(e){m.all(e).catch(x)})),"Promise",{all:function(e){var t=this,r=S(t),n=r.resolve,o=r.reject,i=O(function(){var r=[],i=0,a=1;h(e,!1,function(e){var s=i++,u=!1;r.push(void 0),a++,t.resolve(e).then(function(e){u||(u=!0,r[s]=e,--a||n(r))},o)}),--a||n(r)});return i&&o(i.error),r.promise},race:function(e){var t=this,r=S(t),n=r.reject,o=O(function(){h(e,!1,function(e){t.resolve(e).then(r.resolve,n)})});return o&&n(o.error),r.promise}})},{"./_a-function":21,"./_an-instance":23,"./_classof":26,"./_core":28,"./_ctx":29,"./_export":35,"./_for-of":37,"./_global":38,"./_is-object":48,"./_iter-detect":52,"./_library":56,"./_microtask":58,"./_redefine-all":72,"./_set-species":75,"./_set-to-string-tag":76,"./_species-constructor":79,"./_task":81,"./_wks":91}],101:[function(e,t,r){"use strict";var n=e("./_string-at")(!0);e("./_iter-define")(String,"String",function(e){this._t=String(e),this._i=0},function(){var e,t=this._t,r=this._i;return r>=t.length?{value:void 0,done:!0}:(e=n(t,r),this._i+=e.length,{value:e,done:!1})})},{"./_iter-define":51,"./_string-at":80}],102:[function(e,t,r){"use strict";var n=e("./_global"),o=e("./_has"),i=e("./_descriptors"),a=e("./_export"),s=e("./_redefine"),u=e("./_meta").KEY,c=e("./_fails"),f=e("./_shared"),l=e("./_set-to-string-tag"),p=e("./_uid"),d=e("./_wks"),h=e("./_wks-ext"),b=e("./_wks-define"),v=e("./_keyof"),y=e("./_enum-keys"),_=e("./_is-array"),g=e("./_an-object"),m=e("./_to-iobject"),k=e("./_to-primitive"),x=e("./_property-desc"),w=e("./_object-create"),j=e("./_object-gopn-ext"),E=e("./_object-gopd"),S=e("./_object-dp"),A=e("./_object-keys"),O=E.f,T=S.f,P=j.f,M=n.Symbol,Y=n.JSON,N=Y&&Y.stringify,D=d("_hidden"),F=d("toPrimitive"),L={}.propertyIsEnumerable,I=f("symbol-registry"),R=f("symbols"),$=f("op-symbols"),C=Object.prototype,U="function"==typeof M,G=n.QObject,q=!G||!G.prototype||!G.prototype.findChild,J=i&&c(function(){return 7!=w(T({},"a",{get:function(){return T(this,"a",{value:7}).a}})).a})?function(e,t,r){var n=O(C,t);n&&delete C[t],T(e,t,r),n&&e!==C&&T(C,t,n)}:T,W=function(e){var t=R[e]=w(M.prototype);return t._k=e,t},B=U&&"symbol"==typeof M.iterator?function(e){return"symbol"==typeof e}:function(e){return e instanceof M},K=function(e,t,r){return e===C&&K($,t,r),g(e),t=k(t,!0),g(r),o(R,t)?(r.enumerable?(o(e,D)&&e[D][t]&&(e[D][t]=!1),r=w(r,{enumerable:x(0,!1)})):(o(e,D)||T(e,D,x(1,{})),e[D][t]=!0),J(e,t,r)):T(e,t,r)},z=function(e,t){g(e);for(var r,n=y(t=m(t)),o=0,i=n.length;i>o;)K(e,r=n[o++],t[r]);return e},H=function(e){var t=L.call(this,e=k(e,!0));return!(this===C&&o(R,e)&&!o($,e))&&(!(t||!o(this,e)||!o(R,e)||o(this,D)&&this[D][e])||t)},Q=function(e,t){if(e=m(e),t=k(t,!0),e!==C||!o(R,t)||o($,t)){var r=O(e,t);return!r||!o(R,t)||o(e,D)&&e[D][t]||(r.enumerable=!0),r}},V=function(e){for(var t,r=P(m(e)),n=[],i=0;r.length>i;)o(R,t=r[i++])||t==D||t==u||n.push(t);return n},X=function(e){for(var t,r=e===C,n=P(r?$:m(e)),i=[],a=0;n.length>a;)!o(R,t=n[a++])||r&&!o(C,t)||i.push(R[t]);return i};U||(s((M=function(){if(this instanceof M)throw TypeError("Symbol is not a constructor!");var e=p(arguments.length>0?arguments[0]:void 0),t=function(r){this===C&&t.call($,r),o(this,D)&&o(this[D],e)&&(this[D][e]=!1),J(this,e,x(1,r))};return i&&q&&J(C,e,{configurable:!0,set:t}),W(e)}).prototype,"toString",function(){return this._k}),E.f=Q,S.f=K,e("./_object-gopn").f=j.f=V,e("./_object-pie").f=H,e("./_object-gops").f=X,i&&!e("./_library")&&s(C,"propertyIsEnumerable",H,!0),h.f=function(e){return W(d(e))}),a(a.G+a.W+a.F*!U,{Symbol:M});for(var Z="hasInstance,isConcatSpreadable,iterator,match,replace,search,species,split,toPrimitive,toStringTag,unscopables".split(","),ee=0;Z.length>ee;)d(Z[ee++]);for(var Z=A(d.store),ee=0;Z.length>ee;)b(Z[ee++]);a(a.S+a.F*!U,"Symbol",{for:function(e){return o(I,e+="")?I[e]:I[e]=M(e)},keyFor:function(e){if(B(e))return v(I,e);throw TypeError(e+" is not a symbol!")},useSetter:function(){q=!0},useSimple:function(){q=!1}}),a(a.S+a.F*!U,"Object",{create:function(e,t){return void 0===t?w(e):z(w(e),t)},defineProperty:K,defineProperties:z,getOwnPropertyDescriptor:Q,getOwnPropertyNames:V,getOwnPropertySymbols:X}),Y&&a(a.S+a.F*(!U||c(function(){var e=M();return"[null]"!=N([e])||"{}"!=N({a:e})||"{}"!=N(Object(e))})),"JSON",{stringify:function(e){if(void 0!==e&&!B(e)){for(var t,r,n=[e],o=1;arguments.length>o;)n.push(arguments[o++]);return"function"==typeof(t=n[1])&&(r=t),!r&&_(t)||(t=function(e,t){if(r&&(t=r.call(this,e,t)),!B(t))return t}),n[1]=t,N.apply(Y,n)}}}),M.prototype[F]||e("./_hide")(M.prototype,F,M.prototype.valueOf),l(M,"Symbol"),l(Math,"Math",!0),l(n.JSON,"JSON",!0)},{"./_an-object":24,"./_descriptors":31,"./_enum-keys":34,"./_export":35,"./_fails":36,"./_global":38,"./_has":39,"./_hide":40,"./_is-array":46,"./_keyof":55,"./_library":56,"./_meta":57,"./_object-create":59,"./_object-dp":60,"./_object-gopd":62,"./_object-gopn":64,"./_object-gopn-ext":63,"./_object-gops":65,"./_object-keys":68,"./_object-pie":69,"./_property-desc":71,"./_redefine":73,"./_set-to-string-tag":76,"./_shared":78,"./_to-iobject":84,"./_to-primitive":87,"./_uid":88,"./_wks":91,"./_wks-define":89,"./_wks-ext":90}],103:[function(e,t,r){e("./_wks-define")("asyncIterator")},{"./_wks-define":89}],104:[function(e,t,r){e("./_wks-define")("observable")},{"./_wks-define":89}],105:[function(e,t,r){e("./es6.array.iterator");for(var n=e("./_global"),o=e("./_hide"),i=e("./_iterators"),a=e("./_wks")("toStringTag"),s=["NodeList","DOMTokenList","MediaList","StyleSheetList","CSSRuleList"],u=0;u<5;u++){var c=s[u],f=n[c],l=f&&f.prototype;l&&!l[a]&&o(l,a,c),i[c]=i.Array}},{"./_global":38,"./_hide":40,"./_iterators":54,"./_wks":91,"./es6.array.iterator":94}],106:[function(e,t,r){(function(r){var n="object"==typeof r?r:"object"==typeof window?window:"object"==typeof self?self:this,o=n.regeneratorRuntime&&Object.getOwnPropertyNames(n).indexOf("regeneratorRuntime")>=0,i=o&&n.regeneratorRuntime;if(n.regeneratorRuntime=void 0,t.exports=e("./runtime"),o)n.regeneratorRuntime=i;else try{delete n.regeneratorRuntime}catch(e){n.regeneratorRuntime=void 0}}).call(this,"undefined"!=typeof global?global:"undefined"!=typeof self?self:"undefined"!=typeof window?window:{})},{"./runtime":107}],107:[function(e,t,r){(function(e){!function(e){"use strict";function r(e,t,r,n){var i=t&&t.prototype instanceof o?t:o,a=Object.create(i.prototype),s=new d(n||[]);return a._invoke=c(e,r,s),a}function n(e,t,r){try{return{type:"normal",arg:e.call(t,r)}}catch(e){return{type:"throw",arg:e}}}function o(){}function i(){}function a(){}function s(e){["next","throw","return"].forEach(function(t){e[t]=function(e){return this._invoke(t,e)}})}function u(t){function r(e,o,i,a){var s=n(t[e],t,o);if("throw"!==s.type){var u=s.arg,c=u.value;return c&&"object"==typeof c&&_.call(c,"__await")?Promise.resolve(c.__await).then(function(e){r("next",e,i,a)},function(e){r("throw",e,i,a)}):Promise.resolve(c).then(function(e){u.value=e,i(u)},a)}a(s.arg)}"object"==typeof e.process&&e.process.domain&&(r=e.process.domain.bind(r));var o;this._invoke=function(e,t){function n(){return new Promise(function(n,o){r(e,t,n,o)})}return o=o?o.then(n,n):n()}}function c(e,t,r){var o=E;return function(i,a){if(o===A)throw new Error("Generator is already running");if(o===O){if("throw"===i)throw a;return b()}for(r.method=i,r.arg=a;;){var s=r.delegate;if(s){var u=f(s,r);if(u){if(u===T)continue;return u}}if("next"===r.method)r.sent=r._sent=r.arg;else if("throw"===r.method){if(o===E)throw o=O,r.arg;r.dispatchException(r.arg)}else"return"===r.method&&r.abrupt("return",r.arg);o=A;var c=n(e,t,r);if("normal"===c.type){if(o=r.done?O:S,c.arg===T)continue;return{value:c.arg,done:r.done}}"throw"===c.type&&(o=O,r.method="throw",r.arg=c.arg)}}}function f(e,t){var r=e.iterator[t.method];if(r===v){if(t.delegate=null,"throw"===t.method){if(e.iterator.return&&(t.method="return",t.arg=v,f(e,t),"throw"===t.method))return T;t.method="throw",t.arg=new TypeError("The iterator does not provide a 'throw' method")}return T}var o=n(r,e.iterator,t.arg);if("throw"===o.type)return t.method="throw",t.arg=o.arg,t.delegate=null,T;var i=o.arg;return i?i.done?(t[e.resultName]=i.value,t.next=e.nextLoc,"return"!==t.method&&(t.method="next",t.arg=v),t.delegate=null,T):i:(t.method="throw",t.arg=new TypeError("iterator result is not an object"),t.delegate=null,T)}function l(e){var t={tryLoc:e[0]};1 in e&&(t.catchLoc=e[1]),2 in e&&(t.finallyLoc=e[2],t.afterLoc=e[3]),this.tryEntries.push(t)}function p(e){var t=e.completion||{};t.type="normal",delete t.arg,e.completion=t}function d(e){this.tryEntries=[{tryLoc:"root"}],e.forEach(l,this),this.reset(!0)}function h(e){if(e){var t=e[m];if(t)return t.call(e);if("function"==typeof e.next)return e;if(!isNaN(e.length)){var r=-1,n=function t(){for(;++r<e.length;)if(_.call(e,r))return t.value=e[r],t.done=!1,t;return t.value=v,t.done=!0,t};return n.next=n}}return{next:b}}function b(){return{value:v,done:!0}}var v,y=Object.prototype,_=y.hasOwnProperty,g="function"==typeof Symbol?Symbol:{},m=g.iterator||"@@iterator",k=g.asyncIterator||"@@asyncIterator",x=g.toStringTag||"@@toStringTag",w="object"==typeof t,j=e.regeneratorRuntime;if(j)w&&(t.exports=j);else{(j=e.regeneratorRuntime=w?t.exports:{}).wrap=r;var E="suspendedStart",S="suspendedYield",A="executing",O="completed",T={},P={};P[m]=function(){return this};var M=Object.getPrototypeOf,Y=M&&M(M(h([])));Y&&Y!==y&&_.call(Y,m)&&(P=Y);var N=a.prototype=o.prototype=Object.create(P);i.prototype=N.constructor=a,a.constructor=i,a[x]=i.displayName="GeneratorFunction",j.isGeneratorFunction=function(e){var t="function"==typeof e&&e.constructor;return!!t&&(t===i||"GeneratorFunction"===(t.displayName||t.name))},j.mark=function(e){return Object.setPrototypeOf?Object.setPrototypeOf(e,a):(e.__proto__=a,x in e||(e[x]="GeneratorFunction")),e.prototype=Object.create(N),e},j.awrap=function(e){return{__await:e}},s(u.prototype),u.prototype[k]=function(){return this},j.AsyncIterator=u,j.async=function(e,t,n,o){var i=new u(r(e,t,n,o));return j.isGeneratorFunction(t)?i:i.next().then(function(e){return e.done?e.value:i.next()})},s(N),N[x]="Generator",N[m]=function(){return this},N.toString=function(){return"[object Generator]"},j.keys=function(e){var t=[];for(var r in e)t.push(r);return t.reverse(),function r(){for(;t.length;){var n=t.pop();if(n in e)return r.value=n,r.done=!1,r}return r.done=!0,r}},j.values=h,d.prototype={constructor:d,reset:function(e){if(this.prev=0,this.next=0,this.sent=this._sent=v,this.done=!1,this.delegate=null,this.method="next",this.arg=v,this.tryEntries.forEach(p),!e)for(var t in this)"t"===t.charAt(0)&&_.call(this,t)&&!isNaN(+t.slice(1))&&(this[t]=v)},stop:function(){this.done=!0;var e=this.tryEntries[0].completion;if("throw"===e.type)throw e.arg;return this.rval},dispatchException:function(e){function t(t,n){return i.type="throw",i.arg=e,r.next=t,n&&(r.method="next",r.arg=v),!!n}if(this.done)throw e;for(var r=this,n=this.tryEntries.length-1;n>=0;--n){var o=this.tryEntries[n],i=o.completion;if("root"===o.tryLoc)return t("end");if(o.tryLoc<=this.prev){var a=_.call(o,"catchLoc"),s=_.call(o,"finallyLoc");if(a&&s){if(this.prev<o.catchLoc)return t(o.catchLoc,!0);if(this.prev<o.finallyLoc)return t(o.finallyLoc)}else if(a){if(this.prev<o.catchLoc)return t(o.catchLoc,!0)}else{if(!s)throw new Error("try statement without catch or finally");if(this.prev<o.finallyLoc)return t(o.finallyLoc)}}}},abrupt:function(e,t){for(var r=this.tryEntries.length-1;r>=0;--r){var n=this.tryEntries[r];if(n.tryLoc<=this.prev&&_.call(n,"finallyLoc")&&this.prev<n.finallyLoc){var o=n;break}}o&&("break"===e||"continue"===e)&&o.tryLoc<=t&&t<=o.finallyLoc&&(o=null);var i=o?o.completion:{};return i.type=e,i.arg=t,o?(this.method="next",this.next=o.finallyLoc,T):this.complete(i)},complete:function(e,t){if("throw"===e.type)throw e.arg;return"break"===e.type||"continue"===e.type?this.next=e.arg:"return"===e.type?(this.rval=this.arg=e.arg,this.method="return",this.next="end"):"normal"===e.type&&t&&(this.next=t),T},finish:function(e){for(var t=this.tryEntries.length-1;t>=0;--t){var r=this.tryEntries[t];if(r.finallyLoc===e)return this.complete(r.completion,r.afterLoc),p(r),T}},catch:function(e){for(var t=this.tryEntries.length-1;t>=0;--t){var r=this.tryEntries[t];if(r.tryLoc===e){var n=r.completion;if("throw"===n.type){var o=n.arg;p(r)}return o}}throw new Error("illegal catch attempt")},delegateYield:function(e,t,r){return this.delegate={iterator:h(e),resultName:t,nextLoc:r},"next"===this.method&&(this.arg=v),T}}}}("object"==typeof e?e:"object"==typeof window?window:"object"==typeof self?self:this)}).call(this,"undefined"!=typeof global?global:"undefined"!=typeof self?self:"undefined"!=typeof window?window:{})},{}]},{},[1])(1)});;// jsonata-es5.min.js is prepended to this file as part of the Grunt build

;(function(window) {
    if (typeof window.window != "undefined" && window.document)
    return;
    if (window.require && window.define)
    return;

    if (!window.console) {
        window.console = function() {
            var msgs = Array.prototype.slice.call(arguments, 0);
            postMessage({type: "log", data: msgs});
        };
        window.console.error =
        window.console.warn =
        window.console.log =
        window.console.trace = window.console;
    }
    window.window = window;
    window.ace = window;
    window.onerror = function(message, file, line, col, err) {
        postMessage({type: "error", data: {
            message: message,
            data: err.data,
            file: file,
            line: line,
            col: col,
            stack: err.stack
        }});
    };

    window.normalizeModule = function(parentId, moduleName) {
        // normalize plugin requires
        if (moduleName.indexOf("!") !== -1) {
            var chunks = moduleName.split("!");
            return window.normalizeModule(parentId, chunks[0]) + "!" + window.normalizeModule(parentId, chunks[1]);
        }
        // normalize relative requires
        if (moduleName.charAt(0) == ".") {
            var base = parentId.split("/").slice(0, -1).join("/");
            moduleName = (base ? base + "/" : "") + moduleName;

            while (moduleName.indexOf(".") !== -1 && previous != moduleName) {
                var previous = moduleName;
                moduleName = moduleName.replace(/^\.\//, "").replace(/\/\.\//, "/").replace(/[^\/]+\/\.\.\//, "");
            }
        }

        return moduleName;
    };

    window.require = function require(parentId, id) {
        if (!id) {
            id = parentId;
            parentId = null;
        }
        if (!id.charAt)
        throw new Error("worker.js require() accepts only (parentId, id) as arguments");

        id = window.normalizeModule(parentId, id);

        var module = window.require.modules[id];
        if (module) {
            if (!module.initialized) {
                module.initialized = true;
                module.exports = module.factory().exports;
            }
            return module.exports;
        }

        if (!window.require.tlns)
        return console.log("unable to load " + id);

        var path = resolveModuleId(id, window.require.tlns);
        if (path.slice(-3) != ".js") path += ".js";

        window.require.id = id;
        window.require.modules[id] = {}; // prevent infinite loop on broken modules
        importScripts(path);
        return window.require(parentId, id);
    };
    function resolveModuleId(id, paths) {
        var testPath = id, tail = "";
        while (testPath) {
            var alias = paths[testPath];
            if (typeof alias == "string") {
                return alias + tail;
            } else if (alias) {
                return  alias.location.replace(/\/*$/, "/") + (tail || alias.main || alias.name);
            } else if (alias === false) {
                return "";
            }
            var i = testPath.lastIndexOf("/");
            if (i === -1) break;
            tail = testPath.substr(i) + tail;
            testPath = testPath.slice(0, i);
        }
        return id;
    }
    window.require.modules = {};
    window.require.tlns = {};

    window.define = function(id, deps, factory) {
        if (arguments.length == 2) {
            factory = deps;
            if (typeof id != "string") {
                deps = id;
                id = window.require.id;
            }
        } else if (arguments.length == 1) {
            factory = id;
            deps = [];
            id = window.require.id;
        }

        if (typeof factory != "function") {
            window.require.modules[id] = {
                exports: factory,
                initialized: true
            };
            return;
        }

        if (!deps.length)
        // If there is no dependencies, we inject "require", "exports" and
        // "module" as dependencies, to provide CommonJS compatibility.
        deps = ["require", "exports", "module"];

        var req = function(childId) {
            return window.require(id, childId);
        };

        window.require.modules[id] = {
            exports: {},
            factory: function() {
                var module = this;
                var returnExports = factory.apply(this, deps.map(function(dep) {
                    switch (dep) {
                        // Because "require", "exports" and "module" aren't actual
                        // dependencies, we must handle them seperately.
                        case "require": return req;
                        case "exports": return module.exports;
                        case "module":  return module;
                        // But for all other dependencies, we can just go ahead and
                        // require them.
                        default:        return req(dep);
                    }
                }));
                if (returnExports)
                module.exports = returnExports;
                return module;
            }
        };
    };
    window.define.amd = {};
    require.tlns = {};
    window.initBaseUrls  = function initBaseUrls(topLevelNamespaces) {
        for (var i in topLevelNamespaces)
        require.tlns[i] = topLevelNamespaces[i];
    };

    window.initSender = function initSender() {

        var EventEmitter = window.require("ace/lib/event_emitter").EventEmitter;
        var oop = window.require("ace/lib/oop");

        var Sender = function() {};

        (function() {

            oop.implement(this, EventEmitter);

            this.callback = function(data, callbackId) {
                postMessage({
                    type: "call",
                    id: callbackId,
                    data: data
                });
            };

            this.emit = function(name, data) {
                postMessage({
                    type: "event",
                    name: name,
                    data: data
                });
            };

        }).call(Sender.prototype);

        return new Sender();
    };

    var main = window.main = null;
    var sender = window.sender = null;

    window.onmessage = function(e) {
        var msg = e.data;
        if (msg.event && sender) {
            sender._signal(msg.event, msg.data);
        }
        else if (msg.command) {
            if (main[msg.command])
            main[msg.command].apply(main, msg.args);
            else if (window[msg.command])
            window[msg.command].apply(window, msg.args);
            else
            throw new Error("Unknown command:" + msg.command);
        }
        else if (msg.init) {
            window.initBaseUrls(msg.tlns);
            require("ace/lib/es5-shim");
            sender = window.sender = window.initSender();
            var clazz = require(msg.module)[msg.classname];
            main = window.main = new clazz(sender);
        }
    };
})(this);

define("ace/lib/oop",["require","exports","module"], function(require, exports, module) {
    "use strict";

    exports.inherits = function(ctor, superCtor) {
        ctor.super_ = superCtor;
        ctor.prototype = Object.create(superCtor.prototype, {
            constructor: {
                value: ctor,
                enumerable: false,
                writable: true,
                configurable: true
            }
        });
    };

    exports.mixin = function(obj, mixin) {
        for (var key in mixin) {
            obj[key] = mixin[key];
        }
        return obj;
    };

    exports.implement = function(proto, mixin) {
        exports.mixin(proto, mixin);
    };

});

define("ace/range",["require","exports","module"], function(require, exports, module) {
    "use strict";
    var comparePoints = function(p1, p2) {
        return p1.row - p2.row || p1.column - p2.column;
    };
    var Range = function(startRow, startColumn, endRow, endColumn) {
        this.start = {
            row: startRow,
            column: startColumn
        };

        this.end = {
            row: endRow,
            column: endColumn
        };
    };

    (function() {
        this.isEqual = function(range) {
            return this.start.row === range.start.row &&
            this.end.row === range.end.row &&
            this.start.column === range.start.column &&
            this.end.column === range.end.column;
        };
        this.toString = function() {
            return ("Range: [" + this.start.row + "/" + this.start.column +
            "] -> [" + this.end.row + "/" + this.end.column + "]");
        };

        this.contains = function(row, column) {
            return this.compare(row, column) == 0;
        };
        this.compareRange = function(range) {
            var cmp,
            end = range.end,
            start = range.start;

            cmp = this.compare(end.row, end.column);
            if (cmp == 1) {
                cmp = this.compare(start.row, start.column);
                if (cmp == 1) {
                    return 2;
                } else if (cmp == 0) {
                    return 1;
                } else {
                    return 0;
                }
            } else if (cmp == -1) {
                return -2;
            } else {
                cmp = this.compare(start.row, start.column);
                if (cmp == -1) {
                    return -1;
                } else if (cmp == 1) {
                    return 42;
                } else {
                    return 0;
                }
            }
        };
        this.comparePoint = function(p) {
            return this.compare(p.row, p.column);
        };
        this.containsRange = function(range) {
            return this.comparePoint(range.start) == 0 && this.comparePoint(range.end) == 0;
        };
        this.intersects = function(range) {
            var cmp = this.compareRange(range);
            return (cmp == -1 || cmp == 0 || cmp == 1);
        };
        this.isEnd = function(row, column) {
            return this.end.row == row && this.end.column == column;
        };
        this.isStart = function(row, column) {
            return this.start.row == row && this.start.column == column;
        };
        this.setStart = function(row, column) {
            if (typeof row == "object") {
                this.start.column = row.column;
                this.start.row = row.row;
            } else {
                this.start.row = row;
                this.start.column = column;
            }
        };
        this.setEnd = function(row, column) {
            if (typeof row == "object") {
                this.end.column = row.column;
                this.end.row = row.row;
            } else {
                this.end.row = row;
                this.end.column = column;
            }
        };
        this.inside = function(row, column) {
            if (this.compare(row, column) == 0) {
                if (this.isEnd(row, column) || this.isStart(row, column)) {
                    return false;
                } else {
                    return true;
                }
            }
            return false;
        };
        this.insideStart = function(row, column) {
            if (this.compare(row, column) == 0) {
                if (this.isEnd(row, column)) {
                    return false;
                } else {
                    return true;
                }
            }
            return false;
        };
        this.insideEnd = function(row, column) {
            if (this.compare(row, column) == 0) {
                if (this.isStart(row, column)) {
                    return false;
                } else {
                    return true;
                }
            }
            return false;
        };
        this.compare = function(row, column) {
            if (!this.isMultiLine()) {
                if (row === this.start.row) {
                    return column < this.start.column ? -1 : (column > this.end.column ? 1 : 0);
                }
            }

            if (row < this.start.row)
            return -1;

            if (row > this.end.row)
            return 1;

            if (this.start.row === row)
            return column >= this.start.column ? 0 : -1;

            if (this.end.row === row)
            return column <= this.end.column ? 0 : 1;

            return 0;
        };
        this.compareStart = function(row, column) {
            if (this.start.row == row && this.start.column == column) {
                return -1;
            } else {
                return this.compare(row, column);
            }
        };
        this.compareEnd = function(row, column) {
            if (this.end.row == row && this.end.column == column) {
                return 1;
            } else {
                return this.compare(row, column);
            }
        };
        this.compareInside = function(row, column) {
            if (this.end.row == row && this.end.column == column) {
                return 1;
            } else if (this.start.row == row && this.start.column == column) {
                return -1;
            } else {
                return this.compare(row, column);
            }
        };
        this.clipRows = function(firstRow, lastRow) {
            if (this.end.row > lastRow)
            var end = {row: lastRow + 1, column: 0};
            else if (this.end.row < firstRow)
            var end = {row: firstRow, column: 0};

            if (this.start.row > lastRow)
            var start = {row: lastRow + 1, column: 0};
            else if (this.start.row < firstRow)
            var start = {row: firstRow, column: 0};

            return Range.fromPoints(start || this.start, end || this.end);
        };
        this.extend = function(row, column) {
            var cmp = this.compare(row, column);

            if (cmp == 0)
            return this;
            else if (cmp == -1)
            var start = {row: row, column: column};
            else
            var end = {row: row, column: column};

            return Range.fromPoints(start || this.start, end || this.end);
        };

        this.isEmpty = function() {
            return (this.start.row === this.end.row && this.start.column === this.end.column);
        };
        this.isMultiLine = function() {
            return (this.start.row !== this.end.row);
        };
        this.clone = function() {
            return Range.fromPoints(this.start, this.end);
        };
        this.collapseRows = function() {
            if (this.end.column == 0)
            return new Range(this.start.row, 0, Math.max(this.start.row, this.end.row-1), 0)
            else
            return new Range(this.start.row, 0, this.end.row, 0)
        };
        this.toScreenRange = function(session) {
            var screenPosStart = session.documentToScreenPosition(this.start);
            var screenPosEnd = session.documentToScreenPosition(this.end);

            return new Range(
                screenPosStart.row, screenPosStart.column,
                screenPosEnd.row, screenPosEnd.column
            );
        };
        this.moveBy = function(row, column) {
            this.start.row += row;
            this.start.column += column;
            this.end.row += row;
            this.end.column += column;
        };

    }).call(Range.prototype);
    Range.fromPoints = function(start, end) {
        return new Range(start.row, start.column, end.row, end.column);
    };
    Range.comparePoints = comparePoints;

    Range.comparePoints = function(p1, p2) {
        return p1.row - p2.row || p1.column - p2.column;
    };


    exports.Range = Range;
});

define("ace/apply_delta",["require","exports","module"], function(require, exports, module) {
    "use strict";

    function throwDeltaError(delta, errorText){
        console.log("Invalid Delta:", delta);
        throw "Invalid Delta: " + errorText;
    }

    function positionInDocument(docLines, position) {
        return position.row    >= 0 && position.row    <  docLines.length &&
        position.column >= 0 && position.column <= docLines[position.row].length;
    }

    function validateDelta(docLines, delta) {
        if (delta.action != "insert" && delta.action != "remove")
        throwDeltaError(delta, "delta.action must be 'insert' or 'remove'");
        if (!(delta.lines instanceof Array))
        throwDeltaError(delta, "delta.lines must be an Array");
        if (!delta.start || !delta.end)
        throwDeltaError(delta, "delta.start/end must be an present");
        var start = delta.start;
        if (!positionInDocument(docLines, delta.start))
        throwDeltaError(delta, "delta.start must be contained in document");
        var end = delta.end;
        if (delta.action == "remove" && !positionInDocument(docLines, end))
        throwDeltaError(delta, "delta.end must contained in document for 'remove' actions");
        var numRangeRows = end.row - start.row;
        var numRangeLastLineChars = (end.column - (numRangeRows == 0 ? start.column : 0));
        if (numRangeRows != delta.lines.length - 1 || delta.lines[numRangeRows].length != numRangeLastLineChars)
        throwDeltaError(delta, "delta.range must match delta lines");
    }

    exports.applyDelta = function(docLines, delta, doNotValidate) {

        var row = delta.start.row;
        var startColumn = delta.start.column;
        var line = docLines[row] || "";
        switch (delta.action) {
            case "insert":
            var lines = delta.lines;
            if (lines.length === 1) {
                docLines[row] = line.substring(0, startColumn) + delta.lines[0] + line.substring(startColumn);
            } else {
                var args = [row, 1].concat(delta.lines);
                docLines.splice.apply(docLines, args);
                docLines[row] = line.substring(0, startColumn) + docLines[row];
                docLines[row + delta.lines.length - 1] += line.substring(startColumn);
            }
            break;
            case "remove":
            var endColumn = delta.end.column;
            var endRow = delta.end.row;
            if (row === endRow) {
                docLines[row] = line.substring(0, startColumn) + line.substring(endColumn);
            } else {
                docLines.splice(
                    row, endRow - row + 1,
                    line.substring(0, startColumn) + docLines[endRow].substring(endColumn)
                );
            }
            break;
        }
    }
});

define("ace/lib/event_emitter",["require","exports","module"], function(require, exports, module) {
    "use strict";

    var EventEmitter = {};
    var stopPropagation = function() { this.propagationStopped = true; };
    var preventDefault = function() { this.defaultPrevented = true; };

    EventEmitter._emit =
    EventEmitter._dispatchEvent = function(eventName, e) {
        this._eventRegistry || (this._eventRegistry = {});
        this._defaultHandlers || (this._defaultHandlers = {});

        var listeners = this._eventRegistry[eventName] || [];
        var defaultHandler = this._defaultHandlers[eventName];
        if (!listeners.length && !defaultHandler)
        return;

        if (typeof e != "object" || !e)
        e = {};

        if (!e.type)
        e.type = eventName;
        if (!e.stopPropagation)
        e.stopPropagation = stopPropagation;
        if (!e.preventDefault)
        e.preventDefault = preventDefault;

        listeners = listeners.slice();
        for (var i=0; i<listeners.length; i++) {
            listeners[i](e, this);
            if (e.propagationStopped)
            break;
        }

        if (defaultHandler && !e.defaultPrevented)
        return defaultHandler(e, this);
    };


    EventEmitter._signal = function(eventName, e) {
        var listeners = (this._eventRegistry || {})[eventName];
        if (!listeners)
        return;
        listeners = listeners.slice();
        for (var i=0; i<listeners.length; i++)
        listeners[i](e, this);
    };

    EventEmitter.once = function(eventName, callback) {
        var _self = this;
        callback && this.addEventListener(eventName, function newCallback() {
            _self.removeEventListener(eventName, newCallback);
            callback.apply(null, arguments);
        });
    };


    EventEmitter.setDefaultHandler = function(eventName, callback) {
        var handlers = this._defaultHandlers
        if (!handlers)
        handlers = this._defaultHandlers = {_disabled_: {}};

        if (handlers[eventName]) {
            var old = handlers[eventName];
            var disabled = handlers._disabled_[eventName];
            if (!disabled)
            handlers._disabled_[eventName] = disabled = [];
            disabled.push(old);
            var i = disabled.indexOf(callback);
            if (i != -1)
            disabled.splice(i, 1);
        }
        handlers[eventName] = callback;
    };
    EventEmitter.removeDefaultHandler = function(eventName, callback) {
        var handlers = this._defaultHandlers
        if (!handlers)
        return;
        var disabled = handlers._disabled_[eventName];

        if (handlers[eventName] == callback) {
            var old = handlers[eventName];
            if (disabled)
            this.setDefaultHandler(eventName, disabled.pop());
        } else if (disabled) {
            var i = disabled.indexOf(callback);
            if (i != -1)
            disabled.splice(i, 1);
        }
    };

    EventEmitter.on =
    EventEmitter.addEventListener = function(eventName, callback, capturing) {
        this._eventRegistry = this._eventRegistry || {};

        var listeners = this._eventRegistry[eventName];
        if (!listeners)
        listeners = this._eventRegistry[eventName] = [];

        if (listeners.indexOf(callback) == -1)
        listeners[capturing ? "unshift" : "push"](callback);
        return callback;
    };

    EventEmitter.off =
    EventEmitter.removeListener =
    EventEmitter.removeEventListener = function(eventName, callback) {
        this._eventRegistry = this._eventRegistry || {};

        var listeners = this._eventRegistry[eventName];
        if (!listeners)
        return;

        var index = listeners.indexOf(callback);
        if (index !== -1)
        listeners.splice(index, 1);
    };

    EventEmitter.removeAllListeners = function(eventName) {
        if (this._eventRegistry) this._eventRegistry[eventName] = [];
    };

    exports.EventEmitter = EventEmitter;

});

define("ace/anchor",["require","exports","module","ace/lib/oop","ace/lib/event_emitter"], function(require, exports, module) {
    "use strict";

    var oop = require("./lib/oop");
    var EventEmitter = require("./lib/event_emitter").EventEmitter;

    var Anchor = exports.Anchor = function(doc, row, column) {
        this.$onChange = this.onChange.bind(this);
        this.attach(doc);

        if (typeof column == "undefined")
        this.setPosition(row.row, row.column);
        else
        this.setPosition(row, column);
    };

    (function() {

        oop.implement(this, EventEmitter);
        this.getPosition = function() {
            return this.$clipPositionToDocument(this.row, this.column);
        };
        this.getDocument = function() {
            return this.document;
        };
        this.$insertRight = false;
        this.onChange = function(delta) {
            if (delta.start.row == delta.end.row && delta.start.row != this.row)
            return;

            if (delta.start.row > this.row)
            return;

            var point = $getTransformedPoint(delta, {row: this.row, column: this.column}, this.$insertRight);
            this.setPosition(point.row, point.column, true);
        };

        function $pointsInOrder(point1, point2, equalPointsInOrder) {
            var bColIsAfter = equalPointsInOrder ? point1.column <= point2.column : point1.column < point2.column;
            return (point1.row < point2.row) || (point1.row == point2.row && bColIsAfter);
        }

        function $getTransformedPoint(delta, point, moveIfEqual) {
            var deltaIsInsert = delta.action == "insert";
            var deltaRowShift = (deltaIsInsert ? 1 : -1) * (delta.end.row    - delta.start.row);
            var deltaColShift = (deltaIsInsert ? 1 : -1) * (delta.end.column - delta.start.column);
            var deltaStart = delta.start;
            var deltaEnd = deltaIsInsert ? deltaStart : delta.end; // Collapse insert range.
            if ($pointsInOrder(point, deltaStart, moveIfEqual)) {
                return {
                    row: point.row,
                    column: point.column
                };
            }
            if ($pointsInOrder(deltaEnd, point, !moveIfEqual)) {
                return {
                    row: point.row + deltaRowShift,
                    column: point.column + (point.row == deltaEnd.row ? deltaColShift : 0)
                };
            }

            return {
                row: deltaStart.row,
                column: deltaStart.column
            };
        }
        this.setPosition = function(row, column, noClip) {
            var pos;
            if (noClip) {
                pos = {
                    row: row,
                    column: column
                };
            } else {
                pos = this.$clipPositionToDocument(row, column);
            }

            if (this.row == pos.row && this.column == pos.column)
            return;

            var old = {
                row: this.row,
                column: this.column
            };

            this.row = pos.row;
            this.column = pos.column;
            this._signal("change", {
                old: old,
                value: pos
            });
        };
        this.detach = function() {
            this.document.removeEventListener("change", this.$onChange);
        };
        this.attach = function(doc) {
            this.document = doc || this.document;
            this.document.on("change", this.$onChange);
        };
        this.$clipPositionToDocument = function(row, column) {
            var pos = {};

            if (row >= this.document.getLength()) {
                pos.row = Math.max(0, this.document.getLength() - 1);
                pos.column = this.document.getLine(pos.row).length;
            }
            else if (row < 0) {
                pos.row = 0;
                pos.column = 0;
            }
            else {
                pos.row = row;
                pos.column = Math.min(this.document.getLine(pos.row).length, Math.max(0, column));
            }

            if (column < 0)
            pos.column = 0;

            return pos;
        };

    }).call(Anchor.prototype);

});

define("ace/document",["require","exports","module","ace/lib/oop","ace/apply_delta","ace/lib/event_emitter","ace/range","ace/anchor"], function(require, exports, module) {
    "use strict";
    var oop = require("./lib/oop");
    var applyDelta = require("./apply_delta").applyDelta;
    var EventEmitter = require("./lib/event_emitter").EventEmitter;
    var Range = require("./range").Range;
    var Anchor = require("./anchor").Anchor;

    var Document = function(textOrLines) {
        this.$lines = [""];
        if (textOrLines.length === 0) {
            this.$lines = [""];
        } else if (Array.isArray(textOrLines)) {
            this.insertMergedLines({row: 0, column: 0}, textOrLines);
        } else {
            this.insert({row: 0, column:0}, textOrLines);
        }
    };

    (function() {

        oop.implement(this, EventEmitter);
        this.setValue = function(text) {
            var len = this.getLength() - 1;
            this.remove(new Range(0, 0, len, this.getLine(len).length));
            this.insert({row: 0, column: 0}, text);
        };
        this.getValue = function() {
            return this.getAllLines().join(this.getNewLineCharacter());
        };
        this.createAnchor = function(row, column) {
            return new Anchor(this, row, column);
        };
        if ("aaa".split(/a/).length === 0) {
            this.$split = function(text) {
                return text.replace(/\r\n|\r/g, "\n").split("\n");
            };
        } else {
            this.$split = function(text) {
                return text.split(/\r\n|\r|\n/);
            };
        }


        this.$detectNewLine = function(text) {
            var match = text.match(/^.*?(\r\n|\r|\n)/m);
            this.$autoNewLine = match ? match[1] : "\n";
            this._signal("changeNewLineMode");
        };
        this.getNewLineCharacter = function() {
            switch (this.$newLineMode) {
                case "windows":
                return "\r\n";
                case "unix":
                return "\n";
                default:
                return this.$autoNewLine || "\n";
            }
        };

        this.$autoNewLine = "";
        this.$newLineMode = "auto";
        this.setNewLineMode = function(newLineMode) {
            if (this.$newLineMode === newLineMode)
            return;

            this.$newLineMode = newLineMode;
            this._signal("changeNewLineMode");
        };
        this.getNewLineMode = function() {
            return this.$newLineMode;
        };
        this.isNewLine = function(text) {
            return (text == "\r\n" || text == "\r" || text == "\n");
        };
        this.getLine = function(row) {
            return this.$lines[row] || "";
        };
        this.getLines = function(firstRow, lastRow) {
            return this.$lines.slice(firstRow, lastRow + 1);
        };
        this.getAllLines = function() {
            return this.getLines(0, this.getLength());
        };
        this.getLength = function() {
            return this.$lines.length;
        };
        this.getTextRange = function(range) {
            return this.getLinesForRange(range).join(this.getNewLineCharacter());
        };
        this.getLinesForRange = function(range) {
            var lines;
            if (range.start.row === range.end.row) {
                lines = [this.getLine(range.start.row).substring(range.start.column, range.end.column)];
            } else {
                lines = this.getLines(range.start.row, range.end.row);
                lines[0] = (lines[0] || "").substring(range.start.column);
                var l = lines.length - 1;
                if (range.end.row - range.start.row == l)
                lines[l] = lines[l].substring(0, range.end.column);
            }
            return lines;
        };
        this.insertLines = function(row, lines) {
            console.warn("Use of document.insertLines is deprecated. Use the insertFullLines method instead.");
            return this.insertFullLines(row, lines);
        };
        this.removeLines = function(firstRow, lastRow) {
            console.warn("Use of document.removeLines is deprecated. Use the removeFullLines method instead.");
            return this.removeFullLines(firstRow, lastRow);
        };
        this.insertNewLine = function(position) {
            console.warn("Use of document.insertNewLine is deprecated. Use insertMergedLines(position, ['', '']) instead.");
            return this.insertMergedLines(position, ["", ""]);
        };
        this.insert = function(position, text) {
            if (this.getLength() <= 1)
            this.$detectNewLine(text);

            return this.insertMergedLines(position, this.$split(text));
        };
        this.insertInLine = function(position, text) {
            var start = this.clippedPos(position.row, position.column);
            var end = this.pos(position.row, position.column + text.length);

            this.applyDelta({
                start: start,
                end: end,
                action: "insert",
                lines: [text]
            }, true);

            return this.clonePos(end);
        };

        this.clippedPos = function(row, column) {
            var length = this.getLength();
            if (row === undefined) {
                row = length;
            } else if (row < 0) {
                row = 0;
            } else if (row >= length) {
                row = length - 1;
                column = undefined;
            }
            var line = this.getLine(row);
            if (column == undefined)
            column = line.length;
            column = Math.min(Math.max(column, 0), line.length);
            return {row: row, column: column};
        };

        this.clonePos = function(pos) {
            return {row: pos.row, column: pos.column};
        };

        this.pos = function(row, column) {
            return {row: row, column: column};
        };

        this.$clipPosition = function(position) {
            var length = this.getLength();
            if (position.row >= length) {
                position.row = Math.max(0, length - 1);
                position.column = this.getLine(length - 1).length;
            } else {
                position.row = Math.max(0, position.row);
                position.column = Math.min(Math.max(position.column, 0), this.getLine(position.row).length);
            }
            return position;
        };
        this.insertFullLines = function(row, lines) {
            row = Math.min(Math.max(row, 0), this.getLength());
            var column = 0;
            if (row < this.getLength()) {
                lines = lines.concat([""]);
                column = 0;
            } else {
                lines = [""].concat(lines);
                row--;
                column = this.$lines[row].length;
            }
            this.insertMergedLines({row: row, column: column}, lines);
        };
        this.insertMergedLines = function(position, lines) {
            var start = this.clippedPos(position.row, position.column);
            var end = {
                row: start.row + lines.length - 1,
                column: (lines.length == 1 ? start.column : 0) + lines[lines.length - 1].length
            };

            this.applyDelta({
                start: start,
                end: end,
                action: "insert",
                lines: lines
            });

            return this.clonePos(end);
        };
        this.remove = function(range) {
            var start = this.clippedPos(range.start.row, range.start.column);
            var end = this.clippedPos(range.end.row, range.end.column);
            this.applyDelta({
                start: start,
                end: end,
                action: "remove",
                lines: this.getLinesForRange({start: start, end: end})
            });
            return this.clonePos(start);
        };
        this.removeInLine = function(row, startColumn, endColumn) {
            var start = this.clippedPos(row, startColumn);
            var end = this.clippedPos(row, endColumn);

            this.applyDelta({
                start: start,
                end: end,
                action: "remove",
                lines: this.getLinesForRange({start: start, end: end})
            }, true);

            return this.clonePos(start);
        };
        this.removeFullLines = function(firstRow, lastRow) {
            firstRow = Math.min(Math.max(0, firstRow), this.getLength() - 1);
            lastRow  = Math.min(Math.max(0, lastRow ), this.getLength() - 1);
            var deleteFirstNewLine = lastRow == this.getLength() - 1 && firstRow > 0;
            var deleteLastNewLine  = lastRow  < this.getLength() - 1;
            var startRow = ( deleteFirstNewLine ? firstRow - 1                  : firstRow                    );
            var startCol = ( deleteFirstNewLine ? this.getLine(startRow).length : 0                           );
            var endRow   = ( deleteLastNewLine  ? lastRow + 1                   : lastRow                     );
            var endCol   = ( deleteLastNewLine  ? 0                             : this.getLine(endRow).length );
            var range = new Range(startRow, startCol, endRow, endCol);
            var deletedLines = this.$lines.slice(firstRow, lastRow + 1);

            this.applyDelta({
                start: range.start,
                end: range.end,
                action: "remove",
                lines: this.getLinesForRange(range)
            });
            return deletedLines;
        };
        this.removeNewLine = function(row) {
            if (row < this.getLength() - 1 && row >= 0) {
                this.applyDelta({
                    start: this.pos(row, this.getLine(row).length),
                    end: this.pos(row + 1, 0),
                    action: "remove",
                    lines: ["", ""]
                });
            }
        };
        this.replace = function(range, text) {
            if (!(range instanceof Range))
            range = Range.fromPoints(range.start, range.end);
            if (text.length === 0 && range.isEmpty())
            return range.start;
            if (text == this.getTextRange(range))
            return range.end;

            this.remove(range);
            var end;
            if (text) {
                end = this.insert(range.start, text);
            }
            else {
                end = range.start;
            }

            return end;
        };
        this.applyDeltas = function(deltas) {
            for (var i=0; i<deltas.length; i++) {
                this.applyDelta(deltas[i]);
            }
        };
        this.revertDeltas = function(deltas) {
            for (var i=deltas.length-1; i>=0; i--) {
                this.revertDelta(deltas[i]);
            }
        };
        this.applyDelta = function(delta, doNotValidate) {
            var isInsert = delta.action == "insert";
            if (isInsert ? delta.lines.length <= 1 && !delta.lines[0]
                : !Range.comparePoints(delta.start, delta.end)) {
                    return;
                }

                if (isInsert && delta.lines.length > 20000)
                this.$splitAndapplyLargeDelta(delta, 20000);
                applyDelta(this.$lines, delta, doNotValidate);
                this._signal("change", delta);
            };

            this.$splitAndapplyLargeDelta = function(delta, MAX) {
                var lines = delta.lines;
                var l = lines.length;
                var row = delta.start.row;
                var column = delta.start.column;
                var from = 0, to = 0;
                do {
                    from = to;
                    to += MAX - 1;
                    var chunk = lines.slice(from, to);
                    if (to > l) {
                        delta.lines = chunk;
                        delta.start.row = row + from;
                        delta.start.column = column;
                        break;
                    }
                    chunk.push("");
                    this.applyDelta({
                        start: this.pos(row + from, column),
                        end: this.pos(row + to, column = 0),
                        action: delta.action,
                        lines: chunk
                    }, true);
                } while(true);
            };
            this.revertDelta = function(delta) {
                this.applyDelta({
                    start: this.clonePos(delta.start),
                    end: this.clonePos(delta.end),
                    action: (delta.action == "insert" ? "remove" : "insert"),
                    lines: delta.lines.slice()
                });
            };
            this.indexToPosition = function(index, startRow) {
                var lines = this.$lines || this.getAllLines();
                var newlineLength = this.getNewLineCharacter().length;
                for (var i = startRow || 0, l = lines.length; i < l; i++) {
                    index -= lines[i].length + newlineLength;
                    if (index < 0)
                    return {row: i, column: index + lines[i].length + newlineLength};
                }
                return {row: l-1, column: lines[l-1].length};
            };
            this.positionToIndex = function(pos, startRow) {
                var lines = this.$lines || this.getAllLines();
                var newlineLength = this.getNewLineCharacter().length;
                var index = 0;
                var row = Math.min(pos.row, lines.length);
                for (var i = startRow || 0; i < row; ++i)
                index += lines[i].length + newlineLength;

                return index + pos.column;
            };

        }).call(Document.prototype);

        exports.Document = Document;
    });

    define("ace/lib/lang",["require","exports","module"], function(require, exports, module) {
        "use strict";

        exports.last = function(a) {
            return a[a.length - 1];
        };

        exports.stringReverse = function(string) {
            return string.split("").reverse().join("");
        };

        exports.stringRepeat = function (string, count) {
            var result = '';
            while (count > 0) {
                if (count & 1)
                result += string;

                if (count >>= 1)
                string += string;
            }
            return result;
        };

        var trimBeginRegexp = /^\s\s*/;
        var trimEndRegexp = /\s\s*$/;

        exports.stringTrimLeft = function (string) {
            return string.replace(trimBeginRegexp, '');
        };

        exports.stringTrimRight = function (string) {
            return string.replace(trimEndRegexp, '');
        };

        exports.copyObject = function(obj) {
            var copy = {};
            for (var key in obj) {
                copy[key] = obj[key];
            }
            return copy;
        };

        exports.copyArray = function(array){
            var copy = [];
            for (var i=0, l=array.length; i<l; i++) {
                if (array[i] && typeof array[i] == "object")
                copy[i] = this.copyObject(array[i]);
                else
                copy[i] = array[i];
            }
            return copy;
        };

        exports.deepCopy = function deepCopy(obj) {
            if (typeof obj !== "object" || !obj)
            return obj;
            var copy;
            if (Array.isArray(obj)) {
                copy = [];
                for (var key = 0; key < obj.length; key++) {
                    copy[key] = deepCopy(obj[key]);
                }
                return copy;
            }
            if (Object.prototype.toString.call(obj) !== "[object Object]")
            return obj;

            copy = {};
            for (var key in obj)
            copy[key] = deepCopy(obj[key]);
            return copy;
        };

        exports.arrayToMap = function(arr) {
            var map = {};
            for (var i=0; i<arr.length; i++) {
                map[arr[i]] = 1;
            }
            return map;

        };

        exports.createMap = function(props) {
            var map = Object.create(null);
            for (var i in props) {
                map[i] = props[i];
            }
            return map;
        };
        exports.arrayRemove = function(array, value) {
            for (var i = 0; i <= array.length; i++) {
                if (value === array[i]) {
                    array.splice(i, 1);
                }
            }
        };

        exports.escapeRegExp = function(str) {
            return str.replace(/([.*+?^${}()|[\]\/\\])/g, '\\$1');
        };

        exports.escapeHTML = function(str) {
            return str.replace(/&/g, "&#38;").replace(/"/g, "&#34;").replace(/'/g, "&#39;").replace(/</g, "&#60;");
        };

        exports.getMatchOffsets = function(string, regExp) {
            var matches = [];

            string.replace(regExp, function(str) {
                matches.push({
                    offset: arguments[arguments.length-2],
                    length: str.length
                });
            });

            return matches;
        };
        exports.deferredCall = function(fcn) {
            var timer = null;
            var callback = function() {
                timer = null;
                fcn();
            };

            var deferred = function(timeout) {
                deferred.cancel();
                timer = setTimeout(callback, timeout || 0);
                return deferred;
            };

            deferred.schedule = deferred;

            deferred.call = function() {
                this.cancel();
                fcn();
                return deferred;
            };

            deferred.cancel = function() {
                clearTimeout(timer);
                timer = null;
                return deferred;
            };

            deferred.isPending = function() {
                return timer;
            };

            return deferred;
        };


        exports.delayedCall = function(fcn, defaultTimeout) {
            var timer = null;
            var callback = function() {
                timer = null;
                fcn();
            };

            var _self = function(timeout) {
                if (timer == null)
                timer = setTimeout(callback, timeout || defaultTimeout);
            };

            _self.delay = function(timeout) {
                timer && clearTimeout(timer);
                timer = setTimeout(callback, timeout || defaultTimeout);
            };
            _self.schedule = _self;

            _self.call = function() {
                this.cancel();
                fcn();
            };

            _self.cancel = function() {
                timer && clearTimeout(timer);
                timer = null;
            };

            _self.isPending = function() {
                return timer;
            };

            return _self;
        };
    });

    define("ace/worker/mirror",["require","exports","module","ace/range","ace/document","ace/lib/lang"], function(require, exports, module) {
        "use strict";

        var Range = require("../range").Range;
        var Document = require("../document").Document;
        var lang = require("../lib/lang");

        var Mirror = exports.Mirror = function(sender) {
            this.sender = sender;
            var doc = this.doc = new Document("");

            var deferredUpdate = this.deferredUpdate = lang.delayedCall(this.onUpdate.bind(this));

            var _self = this;
            sender.on("change", function(e) {
                var data = e.data;
                if (data[0].start) {
                    doc.applyDeltas(data);
                } else {
                    for (var i = 0; i < data.length; i += 2) {
                        if (Array.isArray(data[i+1])) {
                            var d = {action: "insert", start: data[i], lines: data[i+1]};
                        } else {
                            var d = {action: "remove", start: data[i], end: data[i+1]};
                        }
                        doc.applyDelta(d, true);
                    }
                }
                if (_self.$timeout)
                return deferredUpdate.schedule(_self.$timeout);
                _self.onUpdate();
            });
        };

        (function() {

            this.$timeout = 500;

            this.setTimeout = function(timeout) {
                this.$timeout = timeout;
            };

            this.setValue = function(value) {
                this.doc.setValue(value);
                this.deferredUpdate.schedule(this.$timeout);
            };

            this.getValue = function(callbackId) {
                this.sender.callback(this.doc.getValue(), callbackId);
            };

            this.onUpdate = function() {
            };

            this.isPending = function() {
                return this.deferredUpdate.isPending();
            };

        }).call(Mirror.prototype);

    });

    define("ace/lib/es5-shim",["require","exports","module"], function(require, exports, module) {

        function Empty() {}

        if (!Function.prototype.bind) {
            Function.prototype.bind = function bind(that) { // .length is 1
                var target = this;
                if (typeof target != "function") {
                    throw new TypeError("Function.prototype.bind called on incompatible " + target);
                }
                var args = slice.call(arguments, 1); // for normal call
                var bound = function () {

                    if (this instanceof bound) {

                        var result = target.apply(
                            this,
                            args.concat(slice.call(arguments))
                        );
                        if (Object(result) === result) {
                            return result;
                        }
                        return this;

                    } else {
                        return target.apply(
                            that,
                            args.concat(slice.call(arguments))
                        );

                    }

                };
                if(target.prototype) {
                    Empty.prototype = target.prototype;
                    bound.prototype = new Empty();
                    Empty.prototype = null;
                }
                return bound;
            };
        }
        var call = Function.prototype.call;
        var prototypeOfArray = Array.prototype;
        var prototypeOfObject = Object.prototype;
        var slice = prototypeOfArray.slice;
        var _toString = call.bind(prototypeOfObject.toString);
        var owns = call.bind(prototypeOfObject.hasOwnProperty);
        var defineGetter;
        var defineSetter;
        var lookupGetter;
        var lookupSetter;
        var supportsAccessors;
        if ((supportsAccessors = owns(prototypeOfObject, "__defineGetter__"))) {
            defineGetter = call.bind(prototypeOfObject.__defineGetter__);
            defineSetter = call.bind(prototypeOfObject.__defineSetter__);
            lookupGetter = call.bind(prototypeOfObject.__lookupGetter__);
            lookupSetter = call.bind(prototypeOfObject.__lookupSetter__);
        }
        if ([1,2].splice(0).length != 2) {
            if(function() { // test IE < 9 to splice bug - see issue #138
                function makeArray(l) {
                    var a = new Array(l+2);
                    a[0] = a[1] = 0;
                    return a;
                }
                var array = [], lengthBefore;

                array.splice.apply(array, makeArray(20));
                array.splice.apply(array, makeArray(26));

                lengthBefore = array.length; //46
                array.splice(5, 0, "XXX"); // add one element

                lengthBefore + 1 == array.length

                if (lengthBefore + 1 == array.length) {
                    return true;// has right splice implementation without bugs
                }
            }()) {//IE 6/7
                var array_splice = Array.prototype.splice;
                Array.prototype.splice = function(start, deleteCount) {
                    if (!arguments.length) {
                        return [];
                    } else {
                        return array_splice.apply(this, [
                            start === void 0 ? 0 : start,
                            deleteCount === void 0 ? (this.length - start) : deleteCount
                        ].concat(slice.call(arguments, 2)))
                    }
                };
            } else {//IE8
                Array.prototype.splice = function(pos, removeCount){
                    var length = this.length;
                    if (pos > 0) {
                        if (pos > length)
                        pos = length;
                    } else if (pos == void 0) {
                        pos = 0;
                    } else if (pos < 0) {
                        pos = Math.max(length + pos, 0);
                    }

                    if (!(pos+removeCount < length))
                    removeCount = length - pos;

                    var removed = this.slice(pos, pos+removeCount);
                    var insert = slice.call(arguments, 2);
                    var add = insert.length;
                    if (pos === length) {
                        if (add) {
                            this.push.apply(this, insert);
                        }
                    } else {
                        var remove = Math.min(removeCount, length - pos);
                        var tailOldPos = pos + remove;
                        var tailNewPos = tailOldPos + add - remove;
                        var tailCount = length - tailOldPos;
                        var lengthAfterRemove = length - remove;

                        if (tailNewPos < tailOldPos) { // case A
                            for (var i = 0; i < tailCount; ++i) {
                                this[tailNewPos+i] = this[tailOldPos+i];
                            }
                        } else if (tailNewPos > tailOldPos) { // case B
                            for (i = tailCount; i--; ) {
                                this[tailNewPos+i] = this[tailOldPos+i];
                            }
                        } // else, add == remove (nothing to do)

                        if (add && pos === lengthAfterRemove) {
                            this.length = lengthAfterRemove; // truncate array
                            this.push.apply(this, insert);
                        } else {
                            this.length = lengthAfterRemove + add; // reserves space
                            for (i = 0; i < add; ++i) {
                                this[pos+i] = insert[i];
                            }
                        }
                    }
                    return removed;
                };
            }
        }
        if (!Array.isArray) {
            Array.isArray = function isArray(obj) {
                return _toString(obj) == "[object Array]";
            };
        }
        var boxedString = Object("a"),
        splitString = boxedString[0] != "a" || !(0 in boxedString);

        if (!Array.prototype.forEach) {
            Array.prototype.forEach = function forEach(fun /*, thisp*/) {
                var object = toObject(this),
                self = splitString && _toString(this) == "[object String]" ?
                this.split("") :
                object,
                thisp = arguments[1],
                i = -1,
                length = self.length >>> 0;
                if (_toString(fun) != "[object Function]") {
                    throw new TypeError(); // TODO message
                }

                while (++i < length) {
                    if (i in self) {
                        fun.call(thisp, self[i], i, object);
                    }
                }
            };
        }
        if (!Array.prototype.map) {
            Array.prototype.map = function map(fun /*, thisp*/) {
                var object = toObject(this),
                self = splitString && _toString(this) == "[object String]" ?
                this.split("") :
                object,
                length = self.length >>> 0,
                result = Array(length),
                thisp = arguments[1];
                if (_toString(fun) != "[object Function]") {
                    throw new TypeError(fun + " is not a function");
                }

                for (var i = 0; i < length; i++) {
                    if (i in self)
                    result[i] = fun.call(thisp, self[i], i, object);
                }
                return result;
            };
        }
        if (!Array.prototype.filter) {
            Array.prototype.filter = function filter(fun /*, thisp */) {
                var object = toObject(this),
                self = splitString && _toString(this) == "[object String]" ?
                this.split("") :
                object,
                length = self.length >>> 0,
                result = [],
                value,
                thisp = arguments[1];
                if (_toString(fun) != "[object Function]") {
                    throw new TypeError(fun + " is not a function");
                }

                for (var i = 0; i < length; i++) {
                    if (i in self) {
                        value = self[i];
                        if (fun.call(thisp, value, i, object)) {
                            result.push(value);
                        }
                    }
                }
                return result;
            };
        }
        if (!Array.prototype.every) {
            Array.prototype.every = function every(fun /*, thisp */) {
                var object = toObject(this),
                self = splitString && _toString(this) == "[object String]" ?
                this.split("") :
                object,
                length = self.length >>> 0,
                thisp = arguments[1];
                if (_toString(fun) != "[object Function]") {
                    throw new TypeError(fun + " is not a function");
                }

                for (var i = 0; i < length; i++) {
                    if (i in self && !fun.call(thisp, self[i], i, object)) {
                        return false;
                    }
                }
                return true;
            };
        }
        if (!Array.prototype.some) {
            Array.prototype.some = function some(fun /*, thisp */) {
                var object = toObject(this),
                self = splitString && _toString(this) == "[object String]" ?
                this.split("") :
                object,
                length = self.length >>> 0,
                thisp = arguments[1];
                if (_toString(fun) != "[object Function]") {
                    throw new TypeError(fun + " is not a function");
                }

                for (var i = 0; i < length; i++) {
                    if (i in self && fun.call(thisp, self[i], i, object)) {
                        return true;
                    }
                }
                return false;
            };
        }
        if (!Array.prototype.reduce) {
            Array.prototype.reduce = function reduce(fun /*, initial*/) {
                var object = toObject(this),
                self = splitString && _toString(this) == "[object String]" ?
                this.split("") :
                object,
                length = self.length >>> 0;
                if (_toString(fun) != "[object Function]") {
                    throw new TypeError(fun + " is not a function");
                }
                if (!length && arguments.length == 1) {
                    throw new TypeError("reduce of empty array with no initial value");
                }

                var i = 0;
                var result;
                if (arguments.length >= 2) {
                    result = arguments[1];
                } else {
                    do {
                        if (i in self) {
                            result = self[i++];
                            break;
                        }
                        if (++i >= length) {
                            throw new TypeError("reduce of empty array with no initial value");
                        }
                    } while (true);
                }

                for (; i < length; i++) {
                    if (i in self) {
                        result = fun.call(void 0, result, self[i], i, object);
                    }
                }

                return result;
            };
        }
        if (!Array.prototype.reduceRight) {
            Array.prototype.reduceRight = function reduceRight(fun /*, initial*/) {
                var object = toObject(this),
                self = splitString && _toString(this) == "[object String]" ?
                this.split("") :
                object,
                length = self.length >>> 0;
                if (_toString(fun) != "[object Function]") {
                    throw new TypeError(fun + " is not a function");
                }
                if (!length && arguments.length == 1) {
                    throw new TypeError("reduceRight of empty array with no initial value");
                }

                var result, i = length - 1;
                if (arguments.length >= 2) {
                    result = arguments[1];
                } else {
                    do {
                        if (i in self) {
                            result = self[i--];
                            break;
                        }
                        if (--i < 0) {
                            throw new TypeError("reduceRight of empty array with no initial value");
                        }
                    } while (true);
                }

                do {
                    if (i in this) {
                        result = fun.call(void 0, result, self[i], i, object);
                    }
                } while (i--);

                return result;
            };
        }
        if (!Array.prototype.indexOf || ([0, 1].indexOf(1, 2) != -1)) {
            Array.prototype.indexOf = function indexOf(sought /*, fromIndex */ ) {
                var self = splitString && _toString(this) == "[object String]" ?
                this.split("") :
                toObject(this),
                length = self.length >>> 0;

                if (!length) {
                    return -1;
                }

                var i = 0;
                if (arguments.length > 1) {
                    i = toInteger(arguments[1]);
                }
                i = i >= 0 ? i : Math.max(0, length + i);
                for (; i < length; i++) {
                    if (i in self && self[i] === sought) {
                        return i;
                    }
                }
                return -1;
            };
        }
        if (!Array.prototype.lastIndexOf || ([0, 1].lastIndexOf(0, -3) != -1)) {
            Array.prototype.lastIndexOf = function lastIndexOf(sought /*, fromIndex */) {
                var self = splitString && _toString(this) == "[object String]" ?
                this.split("") :
                toObject(this),
                length = self.length >>> 0;

                if (!length) {
                    return -1;
                }
                var i = length - 1;
                if (arguments.length > 1) {
                    i = Math.min(i, toInteger(arguments[1]));
                }
                i = i >= 0 ? i : length - Math.abs(i);
                for (; i >= 0; i--) {
                    if (i in self && sought === self[i]) {
                        return i;
                    }
                }
                return -1;
            };
        }
        if (!Object.getPrototypeOf) {
            Object.getPrototypeOf = function getPrototypeOf(object) {
                return object.__proto__ || (
                    object.constructor ?
                    object.constructor.prototype :
                    prototypeOfObject
                );
            };
        }
        if (!Object.getOwnPropertyDescriptor) {
            var ERR_NON_OBJECT = "Object.getOwnPropertyDescriptor called on a " +
            "non-object: ";
            Object.getOwnPropertyDescriptor = function getOwnPropertyDescriptor(object, property) {
                if ((typeof object != "object" && typeof object != "function") || object === null)
                throw new TypeError(ERR_NON_OBJECT + object);
                if (!owns(object, property))
                return;

                var descriptor, getter, setter;
                descriptor =  { enumerable: true, configurable: true };
                if (supportsAccessors) {
                    var prototype = object.__proto__;
                    object.__proto__ = prototypeOfObject;

                    var getter = lookupGetter(object, property);
                    var setter = lookupSetter(object, property);
                    object.__proto__ = prototype;

                    if (getter || setter) {
                        if (getter) descriptor.get = getter;
                        if (setter) descriptor.set = setter;
                        return descriptor;
                    }
                }
                descriptor.value = object[property];
                return descriptor;
            };
        }
        if (!Object.getOwnPropertyNames) {
            Object.getOwnPropertyNames = function getOwnPropertyNames(object) {
                return Object.keys(object);
            };
        }
        if (!Object.create) {
            var createEmpty;
            if (Object.prototype.__proto__ === null) {
                createEmpty = function () {
                    return { "__proto__": null };
                };
            } else {
                createEmpty = function () {
                    var empty = {};
                    for (var i in empty)
                    empty[i] = null;
                    empty.constructor =
                    empty.hasOwnProperty =
                    empty.propertyIsEnumerable =
                    empty.isPrototypeOf =
                    empty.toLocaleString =
                    empty.toString =
                    empty.valueOf =
                    empty.__proto__ = null;
                    return empty;
                }
            }

            Object.create = function create(prototype, properties) {
                var object;
                if (prototype === null) {
                    object = createEmpty();
                } else {
                    if (typeof prototype != "object")
                    throw new TypeError("typeof prototype["+(typeof prototype)+"] != 'object'");
                    var Type = function () {};
                    Type.prototype = prototype;
                    object = new Type();
                    object.__proto__ = prototype;
                }
                if (properties !== void 0)
                Object.defineProperties(object, properties);
                return object;
            };
        }

        function doesDefinePropertyWork(object) {
            try {
                Object.defineProperty(object, "sentinel", {});
                return "sentinel" in object;
            } catch (exception) {
            }
        }
        if (Object.defineProperty) {
            var definePropertyWorksOnObject = doesDefinePropertyWork({});
            var definePropertyWorksOnDom = typeof document == "undefined" ||
            doesDefinePropertyWork(document.createElement("div"));
            if (!definePropertyWorksOnObject || !definePropertyWorksOnDom) {
                var definePropertyFallback = Object.defineProperty;
            }
        }

        if (!Object.defineProperty || definePropertyFallback) {
            var ERR_NON_OBJECT_DESCRIPTOR = "Property description must be an object: ";
            var ERR_NON_OBJECT_TARGET = "Object.defineProperty called on non-object: "
            var ERR_ACCESSORS_NOT_SUPPORTED = "getters & setters can not be defined " +
            "on this javascript engine";

            Object.defineProperty = function defineProperty(object, property, descriptor) {
                if ((typeof object != "object" && typeof object != "function") || object === null)
                throw new TypeError(ERR_NON_OBJECT_TARGET + object);
                if ((typeof descriptor != "object" && typeof descriptor != "function") || descriptor === null)
                throw new TypeError(ERR_NON_OBJECT_DESCRIPTOR + descriptor);
                if (definePropertyFallback) {
                    try {
                        return definePropertyFallback.call(Object, object, property, descriptor);
                    } catch (exception) {
                    }
                }
                if (owns(descriptor, "value")) {

                    if (supportsAccessors && (lookupGetter(object, property) ||
                    lookupSetter(object, property)))
                    {
                        var prototype = object.__proto__;
                        object.__proto__ = prototypeOfObject;
                        delete object[property];
                        object[property] = descriptor.value;
                        object.__proto__ = prototype;
                    } else {
                        object[property] = descriptor.value;
                    }
                } else {
                    if (!supportsAccessors)
                    throw new TypeError(ERR_ACCESSORS_NOT_SUPPORTED);
                    if (owns(descriptor, "get"))
                    defineGetter(object, property, descriptor.get);
                    if (owns(descriptor, "set"))
                    defineSetter(object, property, descriptor.set);
                }

                return object;
            };
        }
        if (!Object.defineProperties) {
            Object.defineProperties = function defineProperties(object, properties) {
                for (var property in properties) {
                    if (owns(properties, property))
                    Object.defineProperty(object, property, properties[property]);
                }
                return object;
            };
        }
        if (!Object.seal) {
            Object.seal = function seal(object) {
                return object;
            };
        }
        if (!Object.freeze) {
            Object.freeze = function freeze(object) {
                return object;
            };
        }
        try {
            Object.freeze(function () {});
        } catch (exception) {
            Object.freeze = (function freeze(freezeObject) {
                return function freeze(object) {
                    if (typeof object == "function") {
                        return object;
                    } else {
                        return freezeObject(object);
                    }
                };
            })(Object.freeze);
        }
        if (!Object.preventExtensions) {
            Object.preventExtensions = function preventExtensions(object) {
                return object;
            };
        }
        if (!Object.isSealed) {
            Object.isSealed = function isSealed(object) {
                return false;
            };
        }
        if (!Object.isFrozen) {
            Object.isFrozen = function isFrozen(object) {
                return false;
            };
        }
        if (!Object.isExtensible) {
            Object.isExtensible = function isExtensible(object) {
                if (Object(object) === object) {
                    throw new TypeError(); // TODO message
                }
                var name = '';
                while (owns(object, name)) {
                    name += '?';
                }
                object[name] = true;
                var returnValue = owns(object, name);
                delete object[name];
                return returnValue;
            };
        }
        if (!Object.keys) {
            var hasDontEnumBug = true,
            dontEnums = [
                "toString",
                "toLocaleString",
                "valueOf",
                "hasOwnProperty",
                "isPrototypeOf",
                "propertyIsEnumerable",
                "constructor"
            ],
            dontEnumsLength = dontEnums.length;

            for (var key in {"toString": null}) {
                hasDontEnumBug = false;
            }

            Object.keys = function keys(object) {

                if (
                    (typeof object != "object" && typeof object != "function") ||
                    object === null
                ) {
                    throw new TypeError("Object.keys called on a non-object");
                }

                var keys = [];
                for (var name in object) {
                    if (owns(object, name)) {
                        keys.push(name);
                    }
                }

                if (hasDontEnumBug) {
                    for (var i = 0, ii = dontEnumsLength; i < ii; i++) {
                        var dontEnum = dontEnums[i];
                        if (owns(object, dontEnum)) {
                            keys.push(dontEnum);
                        }
                    }
                }
                return keys;
            };

        }
        if (!Date.now) {
            Date.now = function now() {
                return new Date().getTime();
            };
        }
        var ws = "\x09\x0A\x0B\x0C\x0D\x20\xA0\u1680\u180E\u2000\u2001\u2002\u2003" +
        "\u2004\u2005\u2006\u2007\u2008\u2009\u200A\u202F\u205F\u3000\u2028" +
        "\u2029\uFEFF";
        if (!String.prototype.trim || ws.trim()) {
            ws = "[" + ws + "]";
            var trimBeginRegexp = new RegExp("^" + ws + ws + "*"),
            trimEndRegexp = new RegExp(ws + ws + "*$");
            String.prototype.trim = function trim() {
                return String(this).replace(trimBeginRegexp, "").replace(trimEndRegexp, "");
            };
        }

        function toInteger(n) {
            n = +n;
            if (n !== n) { // isNaN
                n = 0;
            } else if (n !== 0 && n !== (1/0) && n !== -(1/0)) {
                n = (n > 0 || -1) * Math.floor(Math.abs(n));
            }
            return n;
        }

        function isPrimitive(input) {
            var type = typeof input;
            return (
                input === null ||
                type === "undefined" ||
                type === "boolean" ||
                type === "number" ||
                type === "string"
            );
        }

        function toPrimitive(input) {
            var val, valueOf, toString;
            if (isPrimitive(input)) {
                return input;
            }
            valueOf = input.valueOf;
            if (typeof valueOf === "function") {
                val = valueOf.call(input);
                if (isPrimitive(val)) {
                    return val;
                }
            }
            toString = input.toString;
            if (typeof toString === "function") {
                val = toString.call(input);
                if (isPrimitive(val)) {
                    return val;
                }
            }
            throw new TypeError();
        }
        var toObject = function (o) {
            if (o == null) { // this matches both null and undefined
                throw new TypeError("can't convert "+o+" to object");
            }
            return Object(o);
        };

    });
    define("ace/mode/jsonata_worker",["require","exports","ace/lib/oop","ace/worker/mirror"], function(require, exports) {
        var oop = require("../lib/oop");
        var Mirror = require("../worker/mirror").Mirror;

        var JSONataWorker = exports.JSONataWorker = function(sender) {
            Mirror.call(this, sender);
            this.setTimeout(200);
        };

        oop.inherits(JSONataWorker, Mirror);

        (function() {

            this.onUpdate = function() {
                var value = this.doc.getValue();
                var errors = [];
                try {
                    if (value) {
                        jsonata(value);
                    }
                } catch (e) {
                    var pos = this.doc.indexToPosition(e.position-1);
                    var msg = e.message;
                    msg = msg.replace(/ at column \d+/,"");
                    errors.push({
                        row: pos.row,
                        column: pos.column,
                        text: msg,
                        type: "error"
                    });
                }
                this.sender.emit("annotate", errors);
            };

        }).call(JSONataWorker.prototype);

    });
