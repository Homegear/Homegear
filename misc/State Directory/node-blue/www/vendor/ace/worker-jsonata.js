var runtime=function(e){"use strict";var t,r=Object.prototype,n=r.hasOwnProperty,a="function"==typeof Symbol?Symbol:{},i=a.iterator||"@@iterator",o=a.asyncIterator||"@@asyncIterator",s=a.toStringTag||"@@toStringTag";function u(e,t,r,n){var a=t&&t.prototype instanceof g?t:g,i=Object.create(a.prototype),o=new D(n||[]);return i._invoke=function(e,t,r){var n=p;return function(a,i){if(n===l)throw new Error("Generator is already running");if(n===d){if("throw"===a)throw i;return R()}for(r.method=a,r.arg=i;;){var o=r.delegate;if(o){var s=A(o,r);if(s){if(s===h)continue;return s}}if("next"===r.method)r.sent=r._sent=r.arg;else if("throw"===r.method){if(n===p)throw n=d,r.arg;r.dispatchException(r.arg)}else"return"===r.method&&r.abrupt("return",r.arg);n=l;var u=c(e,t,r);if("normal"===u.type){if(n=r.done?d:f,u.arg===h)continue;return{value:u.arg,done:r.done}}"throw"===u.type&&(n=d,r.method="throw",r.arg=u.arg)}}}(e,r,o),i}function c(e,t,r){try{return{type:"normal",arg:e.call(t,r)}}catch(e){return{type:"throw",arg:e}}}e.wrap=u;var p="suspendedStart",f="suspendedYield",l="executing",d="completed",h={};function g(){}function v(){}function b(){}var m={};m[i]=function(){return this};var y=Object.getPrototypeOf,k=y&&y(y(O([])));k&&k!==r&&n.call(k,i)&&(m=k);var x=b.prototype=g.prototype=Object.create(m);function w(e){["next","throw","return"].forEach(function(t){e[t]=function(e){return this._invoke(t,e)}})}function E(e,t){var r;this._invoke=function(a,i){function o(){return new t(function(r,o){!function r(a,i,o,s){var u=c(e[a],e,i);if("throw"!==u.type){var p=u.arg,f=p.value;return f&&"object"==typeof f&&n.call(f,"__await")?t.resolve(f.__await).then(function(e){r("next",e,o,s)},function(e){r("throw",e,o,s)}):t.resolve(f).then(function(e){p.value=e,o(p)},function(e){return r("throw",e,o,s)})}s(u.arg)}(a,i,r,o)})}return r=r?r.then(o,o):o()}}function A(e,r){var n=e.iterator[r.method];if(n===t){if(r.delegate=null,"throw"===r.method){if(e.iterator.return&&(r.method="return",r.arg=t,A(e,r),"throw"===r.method))return h;r.method="throw",r.arg=new TypeError("The iterator does not provide a 'throw' method")}return h}var a=c(n,e.iterator,r.arg);if("throw"===a.type)return r.method="throw",r.arg=a.arg,r.delegate=null,h;var i=a.arg;return i?i.done?(r[e.resultName]=i.value,r.next=e.nextLoc,"return"!==r.method&&(r.method="next",r.arg=t),r.delegate=null,h):i:(r.method="throw",r.arg=new TypeError("iterator result is not an object"),r.delegate=null,h)}function S(e){var t={tryLoc:e[0]};1 in e&&(t.catchLoc=e[1]),2 in e&&(t.finallyLoc=e[2],t.afterLoc=e[3]),this.tryEntries.push(t)}function T(e){var t=e.completion||{};t.type="normal",delete t.arg,e.completion=t}function D(e){this.tryEntries=[{tryLoc:"root"}],e.forEach(S,this),this.reset(!0)}function O(e){if(e){var r=e[i];if(r)return r.call(e);if("function"==typeof e.next)return e;if(!isNaN(e.length)){var a=-1,o=function r(){for(;++a<e.length;)if(n.call(e,a))return r.value=e[a],r.done=!1,r;return r.value=t,r.done=!0,r};return o.next=o}}return{next:R}}function R(){return{value:t,done:!0}}return v.prototype=x.constructor=b,b.constructor=v,b[s]=v.displayName="GeneratorFunction",e.isGeneratorFunction=function(e){var t="function"==typeof e&&e.constructor;return!!t&&(t===v||"GeneratorFunction"===(t.displayName||t.name))},e.mark=function(e){return Object.setPrototypeOf?Object.setPrototypeOf(e,b):(e.__proto__=b,s in e||(e[s]="GeneratorFunction")),e.prototype=Object.create(x),e},e.awrap=function(e){return{__await:e}},w(E.prototype),E.prototype[o]=function(){return this},e.AsyncIterator=E,e.async=function(t,r,n,a,i){void 0===i&&(i=Promise);var o=new E(u(t,r,n,a),i);return e.isGeneratorFunction(r)?o:o.next().then(function(e){return e.done?e.value:o.next()})},w(x),x[s]="Generator",x[i]=function(){return this},x.toString=function(){return"[object Generator]"},e.keys=function(e){var t=[];for(var r in e)t.push(r);return t.reverse(),function r(){for(;t.length;){var n=t.pop();if(n in e)return r.value=n,r.done=!1,r}return r.done=!0,r}},e.values=O,D.prototype={constructor:D,reset:function(e){if(this.prev=0,this.next=0,this.sent=this._sent=t,this.done=!1,this.delegate=null,this.method="next",this.arg=t,this.tryEntries.forEach(T),!e)for(var r in this)"t"===r.charAt(0)&&n.call(this,r)&&!isNaN(+r.slice(1))&&(this[r]=t)},stop:function(){this.done=!0;var e=this.tryEntries[0].completion;if("throw"===e.type)throw e.arg;return this.rval},dispatchException:function(e){if(this.done)throw e;var r=this;function a(n,a){return s.type="throw",s.arg=e,r.next=n,a&&(r.method="next",r.arg=t),!!a}for(var i=this.tryEntries.length-1;i>=0;--i){var o=this.tryEntries[i],s=o.completion;if("root"===o.tryLoc)return a("end");if(o.tryLoc<=this.prev){var u=n.call(o,"catchLoc"),c=n.call(o,"finallyLoc");if(u&&c){if(this.prev<o.catchLoc)return a(o.catchLoc,!0);if(this.prev<o.finallyLoc)return a(o.finallyLoc)}else if(u){if(this.prev<o.catchLoc)return a(o.catchLoc,!0)}else{if(!c)throw new Error("try statement without catch or finally");if(this.prev<o.finallyLoc)return a(o.finallyLoc)}}}},abrupt:function(e,t){for(var r=this.tryEntries.length-1;r>=0;--r){var a=this.tryEntries[r];if(a.tryLoc<=this.prev&&n.call(a,"finallyLoc")&&this.prev<a.finallyLoc){var i=a;break}}i&&("break"===e||"continue"===e)&&i.tryLoc<=t&&t<=i.finallyLoc&&(i=null);var o=i?i.completion:{};return o.type=e,o.arg=t,i?(this.method="next",this.next=i.finallyLoc,h):this.complete(o)},complete:function(e,t){if("throw"===e.type)throw e.arg;return"break"===e.type||"continue"===e.type?this.next=e.arg:"return"===e.type?(this.rval=this.arg=e.arg,this.method="return",this.next="end"):"normal"===e.type&&t&&(this.next=t),h},finish:function(e){for(var t=this.tryEntries.length-1;t>=0;--t){var r=this.tryEntries[t];if(r.finallyLoc===e)return this.complete(r.completion,r.afterLoc),T(r),h}},catch:function(e){for(var t=this.tryEntries.length-1;t>=0;--t){var r=this.tryEntries[t];if(r.tryLoc===e){var n=r.completion;if("throw"===n.type){var a=n.arg;T(r)}return a}}throw new Error("illegal catch attempt")},delegateYield:function(e,r,n){return this.delegate={iterator:O(e),resultName:r,nextLoc:n},"next"===this.method&&(this.arg=t),h}},e}("object"==typeof module?module.exports:{});try{regeneratorRuntime=runtime}catch(e){Function("r","regeneratorRuntime = r")(runtime)}Number.isInteger=Number.isInteger||function(e){return"number"==typeof e&&isFinite(e)&&Math.floor(e)===e},String.fromCodePoint||function(e){var t=function(t){for(var r=[],n=0,a="",i=0,o=arguments.length;i!==o;++i){var s=+arguments[i];if(!(s<1114111&&s>>>0===s))throw RangeError("Invalid code point: "+s);s<=65535?n=r.push(s):(s-=65536,n=r.push(55296+(s>>10),s%1024+56320)),n>=16383&&(a+=e.apply(null,r),r.length=0)}return a+e.apply(null,r)};try{Object.defineProperty(String,"fromCodePoint",{value:t,configurable:!0,writable:!0})}catch(e){String.fromCodePoint=t}}(String.fromCharCode),Object.is||(Object.is=function(e,t){return e===t?0!==e||1/e==1/t:e!=e&&t!=t}),String.prototype.codePointAt||function(){"use strict";var e=function(){try{var e={},t=Object.defineProperty,r=t(e,e,e)&&t}catch(e){}return r}(),t=function(e){if(null==this)throw TypeError();var t=String(this),r=t.length,n=e?Number(e):0;if(n!=n&&(n=0),!(n<0||n>=r)){var a,i=t.charCodeAt(n);return i>=55296&&i<=56319&&r>n+1&&(a=t.charCodeAt(n+1))>=56320&&a<=57343?1024*(i-55296)+a-56320+65536:i}};e?e(String.prototype,"codePointAt",{value:t,configurable:!0,writable:!0}):String.prototype.codePointAt=t}(),Math.log10=Math.log10||function(e){return Math.log(e)*Math.LOG10E},function(e){if("object"==typeof exports&&"undefined"!=typeof module)module.exports=e();else if("function"==typeof define&&define.amd)define([],e);else{("undefined"!=typeof window?window:"undefined"!=typeof global?global:"undefined"!=typeof self?self:this).jsonata=e()}}(function(){return function(){return function e(t,r,n){function a(o,s){if(!r[o]){if(!t[o]){var u="function"==typeof require&&require;if(!s&&u)return u(o,!0);if(i)return i(o,!0);var c=new Error("Cannot find module '"+o+"'");throw c.code="MODULE_NOT_FOUND",c}var p=r[o]={exports:{}};t[o][0].call(p.exports,function(e){return a(t[o][1][e]||e)},p,p.exports,e,t,r,n)}return r[o].exports}for(var i="function"==typeof require&&require,o=0;o<n.length;o++)a(n[o]);return a}}()({1:[function(e,t,r){"use strict";var n=e("./utils"),a=function(){var e=n.stringToArray,t=["Zero","One","Two","Three","Four","Five","Six","Seven","Eight","Nine","Ten","Eleven","Twelve","Thirteen","Fourteen","Fifteen","Sixteen","Seventeen","Eighteen","Nineteen"],r=["Zeroth","First","Second","Third","Fourth","Fifth","Sixth","Seventh","Eighth","Ninth","Tenth","Eleventh","Twelfth","Thirteenth","Fourteenth","Fifteenth","Sixteenth","Seventeenth","Eighteenth","Nineteenth"],a=["Twenty","Thirty","Forty","Fifty","Sixty","Seventy","Eighty","Ninety","Hundred"],i=["Thousand","Million","Billion","Trillion"];var o={};t.forEach(function(e,t){o[e.toLowerCase()]=t}),r.forEach(function(e,t){o[e.toLowerCase()]=t}),a.forEach(function(e,t){var r=e.toLowerCase();o[r]=10*(t+2),o[r.substring(0,e.length-1)+"ieth"]=o[r]}),o.hundredth=100,i.forEach(function(e,t){var r=e.toLowerCase(),n=Math.pow(10,3*(t+1));o[r]=n,o[r+"th"]=n});var s=[[1e3,"m"],[900,"cm"],[500,"d"],[400,"cd"],[100,"c"],[90,"xc"],[50,"l"],[40,"xl"],[10,"x"],[9,"ix"],[5,"v"],[4,"iv"],[1,"i"]],u={M:1e3,D:500,C:100,L:50,X:10,V:5,I:1};function c(e,t){if(void 0!==e)return l(e=Math.floor(e),h(t))}var p={DECIMAL:"decimal",LETTERS:"letters",ROMAN:"roman",WORDS:"words",SEQUENCE:"sequence"},f={UPPER:"upper",LOWER:"lower",TITLE:"title"};function l(n,o){var u,c=n<0;switch(n=Math.abs(n),o.primary){case p.LETTERS:u=function(e,t){for(var r=[],n=t.charCodeAt(0);e>0;)r.unshift(String.fromCharCode((e-1)%26+n)),e=Math.floor((e-1)/26);return r.join("")}(n,o.case===f.UPPER?"A":"a");break;case p.ROMAN:u=function e(t){for(var r=0;r<s.length;r++){var n=s[r];if(t>=n[0])return n[1]+e(t-n[0])}return""}(n),o.case===f.UPPER&&(u=u.toUpperCase());break;case p.WORDS:u=function(e,n){return function e(n,o,s){var u="";if(n<=19)u=(o?" and ":"")+(s?r[n]:t[n]);else if(n<100){var c=Math.floor(n/10),p=n%10;u=(o?" and ":"")+a[c-2],p>0?u+="-"+e(p,!1,s):s&&(u=u.substring(0,u.length-1)+"ieth")}else if(n<1e3){var f=Math.floor(n/100),l=n%100;u=(o?", ":"")+t[f]+" Hundred",l>0?u+=e(l,!0,s):s&&(u+="th")}else{var d=Math.floor(Math.log10(n)/3);d>i.length&&(d=i.length);var h=Math.pow(10,3*d),g=Math.floor(n/h),v=n-g*h;u=(o?", ":"")+e(g,!1,!1)+" "+i[d-1],v>0?u+=e(v,!0,s):s&&(u+="th")}return u}(e,!1,n)}(n,o.ordinal),o.case===f.UPPER?u=u.toUpperCase():o.case===f.LOWER&&(u=u.toLowerCase());break;case p.DECIMAL:u=""+n;var l=o.mandatoryDigits-u.length;if(l>0){var d=new Array(l+1).join("0");u=d+u}if(48!==o.zeroCode&&(u=e(u).map(function(e){return String.fromCodePoint(e.codePointAt(0)+o.zeroCode-48)}).join("")),o.regular)for(var h=Math.floor((u.length-1)/o.groupingSeparators.position);h>0;h--){var g=u.length-h*o.groupingSeparators.position;u=u.substr(0,g)+o.groupingSeparators.character+u.substr(g)}else o.groupingSeparators.reverse().forEach(function(e){var t=u.length-e.position;u=u.substr(0,t)+e.character+u.substr(t)});if(o.ordinal){var v={1:"st",2:"nd",3:"rd"}[u[u.length-1]];(!v||u.length>1&&"1"===u[u.length-2])&&(v="th"),u+=v}break;case p.SEQUENCE:throw{code:"D3130",value:o.token}}return c&&(u="-"+u),u}var d=[48,1632,1776,1984,2406,2534,2662,2790,2918,3046,3174,3302,3430,3558,3664,3792,3872,4160,4240,6112,6160,6470,6608,6784,6800,6992,7088,7232,7248,42528,43216,43264,43472,43504,43600,44016,65296];function h(t){var r,n={type:"integer",primary:p.DECIMAL,case:f.LOWER,ordinal:!1},a=t.lastIndexOf(";");switch(-1===a?r=t:(r=t.substring(0,a),"o"===t.substring(a+1)[0]&&(n.ordinal=!0)),r){case"A":n.case=f.UPPER;case"a":n.primary=p.LETTERS;break;case"I":n.case=f.UPPER;case"i":n.primary=p.ROMAN;break;case"W":n.case=f.UPPER,n.primary=p.WORDS;break;case"Ww":n.case=f.TITLE,n.primary=p.WORDS;break;case"w":n.primary=p.WORDS;break;default:var i=null,o=0,s=0,u=[],c=0;if(e(r).map(function(e){return e.codePointAt(0)}).reverse().forEach(function(e){for(var t=!1,r=0;r<d.length;r++){var n=d[r];if(e>=n&&e<=n+9){if(t=!0,o++,c++,null===i)i=n;else if(n!==i)throw{code:"D3131"};break}}t||(35===e?(c++,s++):u.push({position:c,character:String.fromCodePoint(e)}))}),o>0){n.primary=p.DECIMAL,n.zeroCode=i,n.mandatoryDigits=o,n.optionalDigits=s;var l=function(e){if(0===e.length)return 0;for(var t=e[0].character,r=1;r<e.length;r++)if(e[r].character!==t)return 0;for(var n=e.map(function(e){return e.position}),a=n.reduce(function e(t,r){return 0===r?t:e(r,t%r)}),i=1;i<=n.length;i++)if(-1===n.indexOf(i*a))return 0;return a}(u);l>0?(n.regular=!0,n.groupingSeparators={position:l,character:u[0].character}):(n.regular=!1,n.groupingSeparators=u)}else n.primary=p.SEQUENCE,n.token=r}return n}var g={Y:"1",M:"1",D:"1",d:"1",F:"n",W:"1",w:"1",X:"1",x:"1",H:"1",h:"1",P:"n",m:"01",s:"01",f:"1",Z:"01:01",z:"01:01",C:"n",E:"n"};function v(e){for(var t=[],r={type:"datetime",parts:t},n=function(r,n){if(n>r){var a=e.substring(r,n);a=a.split("]]").join("]"),t.push({type:"literal",value:a})}},a=0,i=0;i<e.length;){if("["===e.charAt(i)){if("["===e.charAt(i+1)){n(a,i),t.push({type:"literal",value:"["}),a=i+=2;continue}if(n(a,i),a=i,-1===(i=e.indexOf("]",a)))throw{code:"D3135"};var o,s=e.substring(a+1,i),u={type:"marker",component:(s=s.split(/\s+/).join("")).charAt(0)},c=s.lastIndexOf(",");if(-1!==c){var p=s.substring(c+1),l=p.indexOf("-"),d=void 0,v=void 0,b=function(e){return void 0===e||"*"===e?void 0:parseInt(e)};-1===l?d=p:(d=p.substring(0,l),v=p.substring(l+1));var m={min:b(d),max:b(v)};u.width=m,o=s.substring(1,c)}else o=s.substring(1);if(1===o.length)u.presentation1=o;else if(o.length>1){var y=o.charAt(o.length-1);-1!=="atco".indexOf(y)?(u.presentation2=y,"o"===y&&(u.ordinal=!0),u.presentation1=o.substring(0,o.length-1)):u.presentation1=o}else u.presentation1=g[u.component];if(void 0===u.presentation1)throw{code:"D3132",value:u.component};if("n"===u.presentation1[0])u.names=f.LOWER;else if("N"===u.presentation1[0])"n"===u.presentation1[1]?u.names=f.TITLE:u.names=f.UPPER;else if(-1!=="YMDdFWwXxHhmsf".indexOf(u.component)){var k=u.presentation1;if(u.presentation2&&(k+=";"+u.presentation2),u.integerFormat=h(k),u.width&&void 0!==u.width.min&&u.integerFormat.mandatoryDigits<u.width.min&&(u.integerFormat.mandatoryDigits=u.width.min),"Y"===u.component)if(u.n=-1,u.width&&void 0!==u.width.max)u.n=u.width.max,u.integerFormat.mandatoryDigits=u.n;else{var x=u.integerFormat.mandatoryDigits+u.integerFormat.optionalDigits;x>=2&&(u.n=x)}}"Z"!==u.component&&"z"!==u.component||(u.integerFormat=h(u.presentation1)),t.push(u),a=i+1}i++}return n(a,i),r}var b=["","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday","Sunday"],m=["January","February","March","April","May","June","July","August","September","October","November","December"],y=function(e){var t=Date.UTC(e.year,e.month),r=new Date(t).getUTCDay();return 0===r&&(r=7),r>4?t+864e5*(8-r):t-864e5*(r-1)},k=function e(t,r){return{year:t,month:r,nextMonth:function(){return 11===r?e(t+1,0):e(t,r+1)},previousMonth:function(){return 0===r?e(t-1,11):e(t,r-1)},nextYear:function(){return e(t+1,r)},previousYear:function(){return e(t-1,r)}}},x=function(e,t){return(t-e)/6048e5+1},w=function(e,t){var r;switch(t){case"Y":r=e.getUTCFullYear();break;case"M":r=e.getUTCMonth()+1;break;case"D":r=e.getUTCDate();break;case"d":r=(Date.UTC(e.getUTCFullYear(),e.getUTCMonth(),e.getUTCDate())-Date.UTC(e.getUTCFullYear(),0))/864e5+1;break;case"F":0===(r=e.getUTCDay())&&(r=7);break;case"W":var n=k(e.getUTCFullYear(),0),a=y(n),i=Date.UTC(n.year,e.getUTCMonth(),e.getUTCDate()),o=x(a,i);if(o>52)i>=y(n.nextYear())&&(o=1);else if(o<1){var s=y(n.previousYear());o=x(s,i)}r=Math.floor(o);break;case"w":var u=k(e.getUTCFullYear(),e.getUTCMonth()),c=y(u),p=Date.UTC(u.year,u.month,e.getUTCDate()),f=x(c,p);if(f>4)p>=y(u.nextMonth())&&(f=1);else if(f<1){var l=y(u.previousMonth());f=x(l,p)}r=Math.floor(f);break;case"X":var d=k(e.getUTCFullYear(),0),h=y(d),g=y(d.nextYear()),v=e.getTime();r=v<h?d.year-1:v>=g?d.year+1:d.year;break;case"x":var b=k(e.getUTCFullYear(),e.getUTCMonth()),m=y(b),w=b.nextMonth(),E=y(w),A=e.getTime();r=A<m?b.previousMonth().month+1:A>=E?w.month+1:b.month+1;break;case"H":r=e.getUTCHours();break;case"h":r=e.getUTCHours(),0===(r%=12)&&(r=12);break;case"P":r=e.getUTCHours()>=12?"pm":"am";break;case"m":r=e.getUTCMinutes();break;case"s":r=e.getUTCSeconds();break;case"f":r=e.getUTCMilliseconds();break;case"Z":case"z":break;case"C":case"E":r="ISO"}return r},E=null;function A(e,t,r){var n=0,a=0;if(void 0!==r){var i=parseInt(r);n=Math.floor(i/100),a=i%100}var o;void 0===t?(null===E&&(E=v("[Y0001]-[M01]-[D01]T[H01]:[m01]:[s01].[f001][Z01:01t]")),o=E):o=v(t);var s=new Date(e+60*(60*n+a)*1e3),u="";return o.parts.forEach(function(e){"literal"===e.type?u+=e.value:u+=function(e,t){var r=w(e,t.component);if(-1!=="YMDdFWwXxHhms".indexOf(t.component))if("Y"===t.component&&-1!==t.n&&(r%=Math.pow(10,t.n)),t.names){if("M"===t.component||"x"===t.component)r=m[r-1];else{if("F"!==t.component)throw{code:"D3133",value:t.component};r=b[r]}t.names===f.UPPER?r=r.toUpperCase():t.names===f.LOWER&&(r=r.toLowerCase()),t.width&&r.length>t.width.max&&(r=r.substring(0,t.width.max))}else r=l(r,t.integerFormat);else if("f"===t.component)r=l(r,t.integerFormat);else if("Z"===t.component||"z"===t.component){var i=100*n+a;if(t.integerFormat.regular)r=l(i,t.integerFormat);else{var o=t.integerFormat.mandatoryDigits;if(1===o||2===o)r=l(n,t.integerFormat),0!==a&&(r+=":"+c(a,"00"));else{if(3!==o&&4!==o)throw{code:"D3134",value:o};r=l(i,t.integerFormat)}}i>=0&&(r="+"+r),"z"===t.component&&(r="GMT"+r),0===i&&"t"===t.presentation2&&(r="Z")}return r}(s,e)}),u}function S(e){var t={};if("datetime"===e.type)t.type="datetime",t.parts=e.parts.map(function(e){var t={};if("literal"===e.type)t.regex=e.value.replace(/[.*+?^${}()|[\]\\]/g,"\\$&");else if(e.integerFormat)t=S(e.integerFormat);else{t.regex="[a-zA-Z]+";var r={};if("M"===e.component||"x"===e.component)m.forEach(function(t,n){e.width&&e.width.max?r[t.substring(0,e.width.max)]=n+1:r[t]=n+1});else if("F"===e.component)b.forEach(function(t,n){n>0&&(e.width&&e.width.max?r[t.substring(0,e.width.max)]=n:r[t]=n)});else{if("P"!==e.component)throw{code:"D3133",value:e.component};r={am:0,AM:0,pm:1,PM:1}}t.parse=function(e){return r[e]}}return t.component=e.component,t});else{t.type="integer";var r=e.case===f.UPPER;switch(e.primary){case p.LETTERS:t.regex=r?"[A-Z]+":"[a-z]+",t.parse=function(e){return function(e,t){for(var r=t.charCodeAt(0),n=0,a=0;a<e.length;a++)n+=(e.charCodeAt(e.length-a-1)-r+1)*Math.pow(26,a);return n}(e,r?"A":"a")};break;case p.ROMAN:t.regex=r?"[MDCLXVI]+":"[mdclxvi]+",t.parse=function(e){return function(e){for(var t=0,r=1,n=e.length-1;n>=0;n--){var a=e[n],i=u[a];i<r?t-=i:(r=i,t+=i)}return t}(r?e:e.toUpperCase())};break;case p.WORDS:t.regex="(?:"+Object.keys(o).concat("and","[\\-, ]").join("|")+")+",t.parse=function(e){return t=e.toLowerCase(),r=t.split(/,\s|\sand\s|[\s\\-]/).map(function(e){return o[e]}),n=[0],r.forEach(function(e){if(e<100){var t=n.pop();t>=1e3&&(n.push(t),t=0),n.push(t+e)}else n.push(n.pop()*e)}),n.reduce(function(e,t){return e+t},0);var t,r,n};break;case p.DECIMAL:t.regex="[0-9]+",e.ordinal&&(t.regex+="(?:th|st|nd|rd)"),t.parse=function(t){var r=t;return e.ordinal&&(r=t.substring(0,t.length-2)),e.regular?r=r.split(",").join(""):e.groupingSeparators.forEach(function(e){r=r.split(e.character).join("")}),48!==e.zeroCode&&(r=r.split("").map(function(t){return String.fromCodePoint(t.codePointAt(0)-e.zeroCode+48)}).join("")),parseInt(r)};break;case p.SEQUENCE:throw{code:"D3130",value:e.token}}}return t}var T=new RegExp("^\\d{4}(-[01]\\d)*(-[0-3]\\d)*(T[0-2]\\d:[0-5]\\d:[0-5]\\d)*(\\.\\d+)?([+-][0-2]\\d:?[0-5]\\d|Z)?$");return{formatInteger:c,parseInteger:function(e,t){if(void 0!==e)return S(h(t)).parse(e)},fromMillis:function(e,t,r){if(void 0!==e)return A.call(this,e,t,r)},toMillis:function(e,t){if(void 0!==e){if(void 0===t){if(!T.test(e))throw{stack:(new Error).stack,code:"D3110",value:e};return Date.parse(e)}return function(e,t){var r=S(v(t)),n="^"+r.parts.map(function(e){return"("+e.regex+")"}).join("")+"$",a=new RegExp(n,"i").exec(e);if(null!==a){for(var i={},o=1;o<a.length;o++){var s=r.parts[o-1];s.parse&&(i[s.component]=s.parse(a[o]))}if(0===Object.getOwnPropertyNames(i).length)return;var u=0,c=function(e){u<<=1,u+=e?1:0},p=function(e){return!(~e&u||!(e&u))};"YXMxWwdD".split("").forEach(function(e){return c(i[e])});var f=!p(161)&&p(130),l=p(84),d=!l&&p(72);u=0,"PHhmsf".split("").forEach(function(e){return c(i[e])});var h=!p(23)&&p(47),g=(f?"YD":l?"XxwF":d?"XWF":"YMD")+(h?"Phmsf":"Hmsf"),b=this.environment.timestamp,m=!1,y=!1;if(g.split("").forEach(function(e){if(void 0===i[e])m?(i[e]=-1!=="MDd".indexOf(e)?1:0,y=!0):i[e]=w(b,e);else if(m=!0,y)throw{code:"D3136"}}),i.M>0?i.M-=1:i.M=0,f){var k=Date.UTC(i.Y,0),x=1e3*(i.d-1)*60*60*24,E=new Date(k+x);i.M=E.getUTCMonth(),i.D=E.getUTCDate()}if(l)throw{code:"D3136"};if(d)throw{code:"D3136"};return h&&(i.H=12===i.h?0:i.h,1===i.P&&(i.H+=12)),Date.UTC(i.Y,i.M,i.D,i.H,i.m,i.s,i.f)}}.call(this,e,t)}}}}();t.exports=a},{"./utils":6}],2:[function(e,t,r){(function(r){"use strict";function n(e){"@babel/helpers - typeof";return(n="function"==typeof Symbol&&"symbol"==typeof Symbol.iterator?function(e){return typeof e}:function(e){return e&&"function"==typeof Symbol&&e.constructor===Symbol&&e!==Symbol.prototype?"symbol":typeof e})(e)}var a=e("./utils"),i=function(){var e=regeneratorRuntime.mark(O),t=regeneratorRuntime.mark(R),i=regeneratorRuntime.mark(Y),o=regeneratorRuntime.mark(P),s=regeneratorRuntime.mark(M),u=regeneratorRuntime.mark(U),c=regeneratorRuntime.mark(N),p=regeneratorRuntime.mark(L),f=regeneratorRuntime.mark(I),l=regeneratorRuntime.mark($),d=regeneratorRuntime.mark(z),h=regeneratorRuntime.mark(q),g=a.isNumeric,v=a.isArrayOfStrings,b=a.isArrayOfNumbers,m=a.createSequence,y=a.isSequence,k=a.isFunction,x=a.isLambda,w=a.isIterable,E=a.getFunctionArity,A=a.isDeepEqual,S=a.stringToArray;function T(e,t,r){if(void 0!==e){var n=S(e),a=n.length;if(a+t<0&&(t=0),void 0!==r){if(r<=0)return"";var i=t>=0?t+r:a+t+r;return n.slice(t,i).join("")}return n.slice(t).join("")}}function D(e){if(void 0!==e)return S(e).length}function O(t,r){var n;return regeneratorRuntime.wrap(function(e){for(;;)switch(e.prev=e.next){case 0:if(n=t.apply(this,[r]),!w(n)){e.next=4;break}return e.delegateYield(n,"t0",3);case 3:n=e.t0;case 4:if(!n||"number"==typeof n.start||"number"===n.end||Array.isArray(n.groups)||k(n.next)){e.next=6;break}throw{code:"T1010",stack:(new Error).stack};case 6:return e.abrupt("return",n);case 7:case"end":return e.stop()}},e,this)}function R(e,r){var n,a;return regeneratorRuntime.wrap(function(t){for(;;)switch(t.prev=t.next){case 0:if(void 0!==e){t.next=2;break}return t.abrupt("return",void 0);case 2:if("string"!=typeof r){t.next=6;break}n=-1!==e.indexOf(r),t.next=9;break;case 6:return t.delegateYield(O(r,e),"t0",7);case 7:a=t.t0,n=void 0!==a;case 9:return t.abrupt("return",n);case 10:case"end":return t.stop()}},t)}function Y(e,t,r){var n,a,o;return regeneratorRuntime.wrap(function(i){for(;;)switch(i.prev=i.next){case 0:if(void 0!==e){i.next=2;break}return i.abrupt("return",void 0);case 2:if(!(r<0)){i.next=4;break}throw{stack:(new Error).stack,value:r,code:"D3040",index:3};case 4:if(n=m(),!(void 0===r||r>0)){i.next=17;break}return a=0,i.delegateYield(O(t,e),"t0",8);case 8:if(void 0===(o=i.t0)){i.next=17;break}case 10:if(void 0===o||!(void 0===r||a<r)){i.next=17;break}return n.push({match:o.match,index:o.start,groups:o.groups}),i.delegateYield(O(o.next),"t1",13);case 13:o=i.t1,a++,i.next=10;break;case 17:return i.abrupt("return",n);case 18:case"end":return i.stop()}},i)}function P(e,t,r,n){var a,i,s,u,c,p,f,l;return regeneratorRuntime.wrap(function(o){for(;;)switch(o.prev=o.next){case 0:if(void 0!==e){o.next=2;break}return o.abrupt("return",void 0);case 2:if(a=this,""!==t){o.next=5;break}throw{code:"D3010",stack:(new Error).stack,value:t,index:2};case 5:if(!(n<0)){o.next=7;break}throw{code:"D3011",stack:(new Error).stack,value:n,index:4};case 7:if(i="string"==typeof r?function(e){for(var t="",n=0,a=r.indexOf("$",n);-1!==a&&n<r.length;){t+=r.substring(n,a),n=a+1;var i=r.charAt(n);if("$"===i)t+="$",n++;else if("0"===i)t+=e.match,n++;else{var o;if(o=0===e.groups.length?1:Math.floor(Math.log(e.groups.length)*Math.LOG10E)+1,a=parseInt(r.substring(n,n+o),10),o>1&&a>e.groups.length&&(a=parseInt(r.substring(n,n+o-1),10)),isNaN(a))t+="$";else{if(e.groups.length>0){var s=e.groups[a-1];void 0!==s&&(t+=s)}n+=a.toString().length}}a=r.indexOf("$",n)}return t+=r.substring(n)}:r,s="",u=0,!(void 0===n||n>0)){o.next=44;break}if(c=0,"string"!=typeof t){o.next=18;break}for(p=e.indexOf(t,u);-1!==p&&(void 0===n||c<n);)s+=e.substring(u,p),s+=r,u=p+t.length,c++,p=e.indexOf(t,u);s+=e.substring(u),o.next=42;break;case 18:return o.delegateYield(O(t,e),"t0",19);case 19:if(void 0===(f=o.t0)){o.next=41;break}case 21:if(void 0===f||!(void 0===n||c<n)){o.next=38;break}if(s+=e.substring(u,f.start),l=i.apply(a,[f]),!w(l)){o.next=27;break}return o.delegateYield(l,"t1",26);case 26:l=o.t1;case 27:if("string"!=typeof l){o.next=31;break}s+=l,o.next=32;break;case 31:throw{code:"D3012",stack:(new Error).stack,value:l};case 32:return u=f.start+f.match.length,c++,o.delegateYield(O(f.next),"t2",35);case 35:f=o.t2,o.next=21;break;case 38:s+=e.substring(u),o.next=42;break;case 41:s=e;case 42:o.next=45;break;case 44:s=e;case 45:return o.abrupt("return",s);case 46:case"end":return o.stop()}},o,this)}function M(e,t,r){var n,a,i,o;return regeneratorRuntime.wrap(function(s){for(;;)switch(s.prev=s.next){case 0:if(void 0!==e){s.next=2;break}return s.abrupt("return",void 0);case 2:if(!(r<0)){s.next=4;break}throw{code:"D3020",stack:(new Error).stack,value:r,index:3};case 4:if(n=[],!(void 0===r||r>0)){s.next=27;break}if("string"!=typeof t){s.next=10;break}n=e.split(t,r),s.next=27;break;case 10:return a=0,s.delegateYield(O(t,e),"t0",12);case 12:if(void 0===(i=s.t0)){s.next=26;break}o=0;case 15:if(void 0===i||!(void 0===r||a<r)){s.next=23;break}return n.push(e.substring(o,i.start)),o=i.end,s.delegateYield(O(i.next),"t1",19);case 19:i=s.t1,a++,s.next=15;break;case 23:(void 0===r||a<r)&&n.push(e.substring(o)),s.next=27;break;case 26:n.push(e);case 27:return s.abrupt("return",n);case 28:case"end":return s.stop()}},s)}function j(e,t){var r;if(void 0!==e){if(t){var n=e.toString().split("e");e=+(n[0]+"e"+(n[1]?+n[1]+t:t))}var a=(r=Math.round(e))-e;return.5===Math.abs(a)&&1===Math.abs(r%2)&&(r-=1),t&&(r=+((n=r.toString().split("e"))[0]+"e"+(n[1]?+n[1]-t:-t))),Object.is(r,-0)&&(r=0),r}}function C(e){if(void 0!==e){var t=!1;if(Array.isArray(e)){if(1===e.length)t=C(e[0]);else if(e.length>1){t=e.filter(function(e){return C(e)}).length>0}}else"string"==typeof e?e.length>0&&(t=!0):g(e)?0!==e&&(t=!0):null!==e&&"object"===n(e)?Object.keys(e).length>0&&(t=!0):"boolean"==typeof e&&!0===e&&(t=!0);return t}}function F(e,t,r,n){var a=[t],i=E(e);return i>=2&&a.push(r),i>=3&&a.push(n),a}function U(e,t){var r,n,a,i;return regeneratorRuntime.wrap(function(o){for(;;)switch(o.prev=o.next){case 0:if(void 0!==e){o.next=2;break}return o.abrupt("return",void 0);case 2:r=m(),n=0;case 4:if(!(n<e.length)){o.next=12;break}return a=F(t,e[n],n,e),o.delegateYield(t.apply(this,a),"t0",7);case 7:void 0!==(i=o.t0)&&r.push(i);case 9:n++,o.next=4;break;case 12:return o.abrupt("return",r);case 13:case"end":return o.stop()}},u,this)}function N(e,t){var r,n,a,i;return regeneratorRuntime.wrap(function(o){for(;;)switch(o.prev=o.next){case 0:if(void 0!==e){o.next=2;break}return o.abrupt("return",void 0);case 2:r=m(),n=0;case 4:if(!(n<e.length)){o.next=13;break}return a=e[n],i=F(t,a,n,e),o.delegateYield(t.apply(this,i),"t0",8);case 8:C(o.t0)&&r.push(a);case 10:n++,o.next=4;break;case 13:return o.abrupt("return",r);case 14:case"end":return o.stop()}},c,this)}function L(e,t){var r,n,a,i,o,s,u;return regeneratorRuntime.wrap(function(c){for(;;)switch(c.prev=c.next){case 0:if(void 0!==e){c.next=2;break}return c.abrupt("return",void 0);case 2:r=!1,a=0;case 4:if(!(a<e.length)){c.next=22;break}if(i=e[a],o=!0,void 0===t){c.next=12;break}return s=F(t,i,a,e),c.delegateYield(t.apply(this,s),"t0",10);case 10:u=c.t0,o=C(u);case 12:if(!o){c.next=19;break}if(r){c.next=18;break}n=i,r=!0,c.next=19;break;case 18:throw{stack:(new Error).stack,code:"D3138",index:a};case 19:a++,c.next=4;break;case 22:if(r){c.next=24;break}throw{stack:(new Error).stack,code:"D3139"};case 24:return c.abrupt("return",n);case 25:case"end":return c.stop()}},p,this)}function I(e,t,r){var n,a,i,o;return regeneratorRuntime.wrap(function(s){for(;;)switch(s.prev=s.next){case 0:if(void 0!==e){s.next=2;break}return s.abrupt("return",void 0);case 2:if(!((a=E(t))<2)){s.next=5;break}throw{stack:(new Error).stack,code:"D3050",index:1};case 5:void 0===r&&e.length>0?(n=e[0],i=1):(n=r,i=0);case 6:if(!(i<e.length)){s.next=15;break}return o=[n,e[i]],a>=3&&o.push(i),a>=4&&o.push(e),s.delegateYield(t.apply(this,o),"t0",11);case 11:n=s.t0,i++,s.next=6;break;case 15:return s.abrupt("return",n);case 16:case"end":return s.stop()}},f,this)}function _(e,t){return void 0===e?t:void 0===t?e:(Array.isArray(e)||(e=m(e)),Array.isArray(t)||(t=[t]),e.concat(t))}function $(e,t){var r,n,a,i;return regeneratorRuntime.wrap(function(o){for(;;)switch(o.prev=o.next){case 0:r=m(),o.t0=regeneratorRuntime.keys(e);case 2:if((o.t1=o.t0()).done){o.next=10;break}return n=o.t1.value,a=F(t,e[n],n,e),o.delegateYield(t.apply(this,a),"t2",6);case 6:void 0!==(i=o.t2)&&r.push(i),o.next=2;break;case 10:return o.abrupt("return",r);case 11:case"end":return o.stop()}},l,this)}function z(e,t){var r,n,a,i;return regeneratorRuntime.wrap(function(o){for(;;)switch(o.prev=o.next){case 0:if(void 0!==e){o.next=2;break}return o.abrupt("return",void 0);case 2:if(!(e.length<=1)){o.next=4;break}return o.abrupt("return",e);case 4:if(void 0!==t){o.next=10;break}if(b(e)||v(e)){o.next=7;break}throw{stack:(new Error).stack,code:"D3070",index:1};case 7:r=regeneratorRuntime.mark(function e(t,r){return regeneratorRuntime.wrap(function(e){for(;;)switch(e.prev=e.next){case 0:return e.abrupt("return",t>r);case 1:case"end":return e.stop()}},e)}),o.next=11;break;case 10:r=t;case 11:return n=regeneratorRuntime.mark(function e(t,n){var a,i;return regeneratorRuntime.wrap(function(e){for(;;)switch(e.prev=e.next){case 0:return a=regeneratorRuntime.mark(function e(t,n,a){return regeneratorRuntime.wrap(function(i){for(;;)switch(i.prev=i.next){case 0:if(0!==n.length){i.next=4;break}Array.prototype.push.apply(t,a),i.next=16;break;case 4:if(0!==a.length){i.next=8;break}Array.prototype.push.apply(t,n),i.next=16;break;case 8:return i.delegateYield(r(n[0],a[0]),"t0",9);case 9:if(!i.t0){i.next=14;break}return t.push(a[0]),i.delegateYield(e(t,n,a.slice(1)),"t1",12);case 12:i.next=16;break;case 14:return t.push(n[0]),i.delegateYield(e(t,n.slice(1),a),"t2",16);case 16:case"end":return i.stop()}},e)}),i=[],e.delegateYield(a(i,t,n),"t0",3);case 3:return e.abrupt("return",i);case 4:case"end":return e.stop()}},e)}),a=regeneratorRuntime.mark(function e(t){var r,a,i;return regeneratorRuntime.wrap(function(o){for(;;)switch(o.prev=o.next){case 0:if(Array.isArray(t)&&!(t.length<=1)){o.next=4;break}return o.abrupt("return",t);case 4:return r=Math.floor(t.length/2),a=t.slice(0,r),i=t.slice(r),o.delegateYield(e(a),"t0",8);case 8:return a=o.t0,o.delegateYield(e(i),"t1",10);case 10:return i=o.t1,o.delegateYield(n(a,i),"t2",12);case 12:return o.abrupt("return",o.t2);case 13:case"end":return o.stop()}},e)}),o.delegateYield(a(e),"t0",14);case 14:return i=o.t0,o.abrupt("return",i);case 16:case"end":return o.stop()}},d)}function q(e,t){var r,n,a,i;return regeneratorRuntime.wrap(function(o){for(;;)switch(o.prev=o.next){case 0:r={},o.t0=regeneratorRuntime.keys(e);case 2:if((o.t1=o.t0()).done){o.next=11;break}return n=o.t1.value,a=e[n],i=F(t,a,n,e),o.delegateYield(t.apply(this,i),"t2",7);case 7:C(o.t2)&&(r[n]=a),o.next=2;break;case 11:return 0===Object.keys(r).length&&(r=void 0),o.abrupt("return",r);case 13:case"end":return o.stop()}},h,this)}return{sum:function(e){if(void 0!==e){var t=0;return e.forEach(function(e){t+=e}),t}},count:function(e){return void 0===e?0:e.length},max:function(e){if(void 0!==e&&0!==e.length)return Math.max.apply(Math,e)},min:function(e){if(void 0!==e&&0!==e.length)return Math.min.apply(Math,e)},average:function(e){if(void 0!==e&&0!==e.length){var t=0;return e.forEach(function(e){t+=e}),t/e.length}},string:function(e){var t=arguments.length>1&&void 0!==arguments[1]&&arguments[1];if(void 0!==e){var r;if("string"==typeof e)r=e;else if(k(e))r="";else{if("number"==typeof e&&!isFinite(e))throw{code:"D3001",value:e,stack:(new Error).stack};var n=t?2:0;Array.isArray(e)&&e.outerWrapper&&(e=e[0]),r=JSON.stringify(e,function(e,t){return null!=t&&t.toPrecision&&g(t)?Number(t.toPrecision(15)):t&&k(t)?"":t},n)}return r}},substring:T,substringBefore:function(e,t){if(void 0!==e){var r=e.indexOf(t);return r>-1?e.substr(0,r):e}},substringAfter:function(e,t){if(void 0!==e){var r=e.indexOf(t);return r>-1?e.substr(r+t.length):e}},lowercase:function(e){if(void 0!==e)return e.toLowerCase()},uppercase:function(e){if(void 0!==e)return e.toUpperCase()},length:D,trim:function(e){if(void 0!==e){var t=e.replace(/[ \t\n\r]+/gm," ");return" "===t.charAt(0)&&(t=t.substring(1))," "===t.charAt(t.length-1)&&(t=t.substring(0,t.length-1)),t}},pad:function(e,t,r){if(void 0!==e){var n;void 0!==r&&0!==r.length||(r=" ");var a=Math.abs(t)-D(e);if(a>0){var i=new Array(a+1).join(r);r.length>1&&(i=T(i,0,a)),n=t>0?e+i:i+e}else n=e;return n}},match:Y,contains:R,replace:P,split:M,join:function(e,t){if(void 0!==e)return void 0===t&&(t=""),e.join(t)},formatNumber:function(e,t,r){if(void 0!==e){var n={"decimal-separator":".","grouping-separator":",","exponent-separator":"e",infinity:"Infinity","minus-sign":"-",NaN:"NaN",percent:"%","per-mille":"‰","zero-digit":"0",digit:"#","pattern-separator":";"};void 0!==r&&Object.keys(r).forEach(function(e){n[e]=r[e]});for(var a=[],i=n["zero-digit"].charCodeAt(0),o=i;o<i+10;o++)a.push(String.fromCharCode(o));var s=a.concat([n["decimal-separator"],n["exponent-separator"],n["grouping-separator"],n.digit,n["pattern-separator"]]),u=t.split(n["pattern-separator"]);if(u.length>2)throw{code:"D3080",stack:(new Error).stack};var c=u.map(function(e){var t,r,a,i,o=function(){for(var t,r=0;r<e.length;r++)if(t=e.charAt(r),-1!==s.indexOf(t)&&t!==n["exponent-separator"])return e.substring(0,r)}(),u=function(){for(var t,r=e.length-1;r>=0;r--)if(t=e.charAt(r),-1!==s.indexOf(t)&&t!==n["exponent-separator"])return e.substring(r+1)}(),c=e.substring(o.length,e.length-u.length),p=e.indexOf(n["exponent-separator"],o.length);-1===p||p>e.length-u.length?(t=c,r=void 0):(t=c.substring(0,p),r=c.substring(p+1));var f=t.indexOf(n["decimal-separator"]);return-1===f?(a=t,i=u):(a=t.substring(0,f),i=t.substring(f+1)),{prefix:o,suffix:u,activePart:c,mantissaPart:t,exponentPart:r,integerPart:a,fractionalPart:i,subpicture:e}});c.forEach(function(e){var t,r,i=e.subpicture,o=i.indexOf(n["decimal-separator"]);o!==i.lastIndexOf(n["decimal-separator"])&&(t="D3081"),i.indexOf(n.percent)!==i.lastIndexOf(n.percent)&&(t="D3082"),i.indexOf(n["per-mille"])!==i.lastIndexOf(n["per-mille"])&&(t="D3083"),-1!==i.indexOf(n.percent)&&-1!==i.indexOf(n["per-mille"])&&(t="D3084");var u=!1;for(r=0;r<e.mantissaPart.length;r++){var c=e.mantissaPart.charAt(r);if(-1!==a.indexOf(c)||c===n.digit){u=!0;break}}u||(t="D3085"),-1!==e.activePart.split("").map(function(e){return-1===s.indexOf(e)?"p":"a"}).join("").indexOf("p")&&(t="D3086"),-1!==o?i.charAt(o-1)!==n["grouping-separator"]&&i.charAt(o+1)!==n["grouping-separator"]||(t="D3087"):e.integerPart.charAt(e.integerPart.length-1)===n["grouping-separator"]&&(t="D3088"),-1!==i.indexOf(n["grouping-separator"]+n["grouping-separator"])&&(t="D3089");var p=e.integerPart.indexOf(n.digit);-1!==p&&e.integerPart.substring(0,p).split("").filter(function(e){return a.indexOf(e)>-1}).length>0&&(t="D3090"),-1!==(p=e.fractionalPart.lastIndexOf(n.digit))&&e.fractionalPart.substring(p).split("").filter(function(e){return a.indexOf(e)>-1}).length>0&&(t="D3091");var f="string"==typeof e.exponentPart;if(f&&e.exponentPart.length>0&&(-1!==i.indexOf(n.percent)||-1!==i.indexOf(n["per-mille"]))&&(t="D3092"),f&&(0===e.exponentPart.length||e.exponentPart.split("").filter(function(e){return-1===a.indexOf(e)}).length>0)&&(t="D3093"),t)throw{code:t,stack:(new Error).stack}});var p,f,l,d,h=c.map(function(e){var t=function(t,r){for(var i=[],o=t.indexOf(n["grouping-separator"]);-1!==o;){var s=(r?t.substring(0,o):t.substring(o)).split("").filter(function(e){return-1!==a.indexOf(e)||e===n.digit}).length;i.push(s),o=e.integerPart.indexOf(n["grouping-separator"],o+1)}return i},r=t(e.integerPart),i=function(e){if(0===e.length)return 0;for(var t=e.reduce(function e(t,r){return 0===r?t:e(r,t%r)}),r=1;r<=e.length;r++)if(-1===e.indexOf(r*t))return 0;return t}(r),o=t(e.fractionalPart,!0),s=e.integerPart.split("").filter(function(e){return-1!==a.indexOf(e)}).length,u=s,c=e.fractionalPart.split(""),p=c.filter(function(e){return-1!==a.indexOf(e)}).length,f=c.filter(function(e){return-1!==a.indexOf(e)||e===n.digit}).length,l="string"==typeof e.exponentPart;0===s&&0===f&&(l?(p=1,f=1):s=1),l&&0===s&&-1!==e.integerPart.indexOf(n.digit)&&(s=1),0===s&&0===p&&(p=1);var d=0;return l&&(d=e.exponentPart.split("").filter(function(e){return-1!==a.indexOf(e)}).length),{integerPartGroupingPositions:r,regularGrouping:i,minimumIntegerPartSize:s,scalingFactor:u,prefix:e.prefix,fractionalPartGroupingPositions:o,minimumFactionalPartSize:p,maximumFactionalPartSize:f,minimumExponentSize:d,suffix:e.suffix,picture:e.subpicture}}),g=n["minus-sign"],v=n["zero-digit"],b=n["decimal-separator"],m=n["grouping-separator"];if(1===h.length&&(h.push(JSON.parse(JSON.stringify(h[0]))),h[1].prefix=g+h[1].prefix),f=-1!==(p=e>=0?h[0]:h[1]).picture.indexOf(n.percent)?100*e:-1!==p.picture.indexOf(n["per-mille"])?1e3*e:e,0===p.minimumExponentSize)l=f;else{var y=Math.pow(10,p.scalingFactor),k=Math.pow(10,p.scalingFactor-1);for(l=f,d=0;l<k;)l*=10,d-=1;for(;l>y;)l/=10,d+=1}var x=function(e,t){var r=Math.abs(e).toFixed(t);return"0"!==v&&(r=r.split("").map(function(e){return e>="0"&&e<="9"?a[e.charCodeAt(0)-48]:e}).join("")),r},w=x(j(l,p.maximumFactionalPartSize),p.maximumFactionalPartSize),E=w.indexOf(".");for(-1===E?w+=b:w=w.replace(".",b);w.charAt(0)===v;)w=w.substring(1);for(;w.charAt(w.length-1)===v;)w=w.substring(0,w.length-1);E=w.indexOf(b);var A=p.minimumIntegerPartSize-E,S=p.minimumFactionalPartSize-(w.length-E-1);if(w=(A>0?new Array(A+1).join(v):"")+w,w+=S>0?new Array(S+1).join(v):"",E=w.indexOf(b),p.regularGrouping>0)for(var T=Math.floor((E-1)/p.regularGrouping),D=1;D<=T;D++)w=[w.slice(0,E-D*p.regularGrouping),m,w.slice(E-D*p.regularGrouping)].join("");else p.integerPartGroupingPositions.forEach(function(e){w=[w.slice(0,E-e),m,w.slice(E-e)].join(""),E++});if(E=w.indexOf(b),p.fractionalPartGroupingPositions.forEach(function(e){w=[w.slice(0,e+E+1),m,w.slice(e+E+1)].join("")}),E=w.indexOf(b),-1!==p.picture.indexOf(b)&&E!==w.length-1||(w=w.substring(0,w.length-1)),void 0!==d){var O=x(d,0);(A=p.minimumExponentSize-O.length)>0&&(O=new Array(A+1).join(v)+O),w=w+n["exponent-separator"]+(d<0?g:"")+O}return w=p.prefix+w+p.suffix}},formatBase:function(e,t){if(void 0!==e){if(e=j(e),(t=void 0===t?10:j(t))<2||t>36)throw{code:"D3100",stack:(new Error).stack,value:t};return e.toString(t)}},number:function(e){var t;if(void 0!==e){if("number"==typeof e)t=e;else if("string"==typeof e&&/^-?[0-9]+(\.[0-9]+)?([Ee][-+]?[0-9]+)?$/.test(e)&&!isNaN(parseFloat(e))&&isFinite(e))t=parseFloat(e);else if(!0===e)t=1;else{if(!1!==e)throw{code:"D3030",value:e,stack:(new Error).stack,index:1};t=0}return t}},floor:function(e){if(void 0!==e)return Math.floor(e)},ceil:function(e){if(void 0!==e)return Math.ceil(e)},round:j,abs:function(e){if(void 0!==e)return Math.abs(e)},sqrt:function(e){if(void 0!==e){if(e<0)throw{stack:(new Error).stack,code:"D3060",index:1,value:e};return Math.sqrt(e)}},power:function(e,t){var r;if(void 0!==e){if(r=Math.pow(e,t),!isFinite(r))throw{stack:(new Error).stack,code:"D3061",index:1,value:e,exp:t};return r}},random:function(){return Math.random()},boolean:C,not:function(e){if(void 0!==e)return!C(e)},map:U,zip:function(){for(var e=[],t=Array.prototype.slice.call(arguments),r=Math.min.apply(Math,t.map(function(e){return Array.isArray(e)?e.length:0})),n=0;n<r;n++){var a=t.map(function(e){return e[n]});e.push(a)}return e},filter:N,single:L,foldLeft:I,sift:q,keys:function e(t){var r=m();if(Array.isArray(t)){var a={};t.forEach(function(t){e(t).forEach(function(e){a[e]=!0})}),r=e(a)}else null===t||"object"!==n(t)||x(t)||Object.keys(t).forEach(function(e){return r.push(e)});return r},lookup:function e(t,r){var a;if(Array.isArray(t)){a=m();for(var i=0;i<t.length;i++){var o=e(t[i],r);void 0!==o&&(Array.isArray(o)?o.forEach(function(e){return a.push(e)}):a.push(o))}}else null!==t&&"object"===n(t)&&(a=t[r]);return a},append:_,exists:function(e){return void 0!==e},spread:function e(t){var r=m();if(Array.isArray(t))t.forEach(function(t){r=_(r,e(t))});else if(null===t||"object"!==n(t)||x(t))r=t;else for(var a in t){var i={};i[a]=t[a],r.push(i)}return r},merge:function(e){if(void 0!==e){var t={};return e.forEach(function(e){for(var r in e)t[r]=e[r]}),t}},reverse:function(e){if(void 0!==e){if(e.length<=1)return e;for(var t=e.length,r=new Array(t),n=0;n<t;n++)r[t-n-1]=e[n];return r}},each:$,error:function(e){throw{code:"D3137",stack:(new Error).stack,message:e||"$error() function evaluated"}},assert:function(e,t){if(!e)throw{code:"D3141",stack:(new Error).stack,message:t||"$assert() statement failed"}},type:function(e){if(void 0!==e)return null===e?"null":g(e)?"number":"string"==typeof e?"string":"boolean"==typeof e?"boolean":Array.isArray(e)?"array":k(e)?"function":"object"},sort:z,shuffle:function(e){if(void 0!==e){if(e.length<=1)return e;for(var t=new Array(e.length),r=0;r<e.length;r++){var n=Math.floor(Math.random()*(r+1));r!==n&&(t[r]=t[n]),t[n]=e[r]}return t}},distinct:function(e){if(void 0!==e){if(!Array.isArray(e)||e.length<=1)return e;for(var t=y(e)?m():[],r=0;r<e.length;r++){for(var n=e[r],a=!1,i=0;i<t.length;i++)if(A(n,t[i])){a=!0;break}a||t.push(n)}return t}},base64encode:function(e){if(void 0!==e)return("undefined"!=typeof window?window.btoa:function(e){return new r.Buffer.from(e,"binary").toString("base64")})(e)},base64decode:function(e){if(void 0!==e)return("undefined"!=typeof window?window.atob:function(e){return new r.Buffer(e,"base64").toString("binary")})(e)},encodeUrlComponent:function(e){if(void 0!==e){var t;try{t=encodeURIComponent(e)}catch(t){throw{code:"D3140",stack:(new Error).stack,value:e,functionName:"encodeUrlComponent"}}return t}},encodeUrl:function(e){if(void 0!==e){var t;try{t=encodeURI(e)}catch(t){throw{code:"D3140",stack:(new Error).stack,value:e,functionName:"encodeUrl"}}return t}},decodeUrlComponent:function(e){if(void 0!==e){var t;try{t=decodeURIComponent(e)}catch(t){throw{code:"D3140",stack:(new Error).stack,value:e,functionName:"decodeUrlComponent"}}return t}},decodeUrl:function(e){if(void 0!==e){var t;try{t=decodeURI(e)}catch(t){throw{code:"D3140",stack:(new Error).stack,value:e,functionName:"decodeUrl"}}return t}}}}();t.exports=i}).call(this,"undefined"!=typeof global?global:"undefined"!=typeof self?self:"undefined"!=typeof window?window:{})},{"./utils":6}],3:[function(e,t,r){"use strict";function n(e){"@babel/helpers - typeof";return(n="function"==typeof Symbol&&"symbol"==typeof Symbol.iterator?function(e){return typeof e}:function(e){return e&&"function"==typeof Symbol&&e.constructor===Symbol&&e!==Symbol.prototype?"symbol":typeof e})(e)}var a=e("./datetime"),i=e("./functions"),o=e("./utils"),s=e("./parser"),u=e("./signature"),c=function(){var e=regeneratorRuntime.mark(L),t=regeneratorRuntime.mark(I),r=regeneratorRuntime.mark($),c=regeneratorRuntime.mark(z),p=regeneratorRuntime.mark(q),f=regeneratorRuntime.mark(W),l=regeneratorRuntime.mark(G),d=regeneratorRuntime.mark(H),h=regeneratorRuntime.mark(ne),g=regeneratorRuntime.mark(oe),v=regeneratorRuntime.mark(se),b=regeneratorRuntime.mark(ue),m=regeneratorRuntime.mark(fe),y=regeneratorRuntime.mark(he),k=regeneratorRuntime.mark(ge),x=regeneratorRuntime.mark(ve),w=regeneratorRuntime.mark(be),E=regeneratorRuntime.mark(ye),A=regeneratorRuntime.mark(xe),S=regeneratorRuntime.mark(Ae),T=regeneratorRuntime.mark(De),D=o.isNumeric,O=o.isArrayOfStrings,R=o.isArrayOfNumbers,Y=o.createSequence,P=o.isSequence,M=o.isFunction,j=o.isLambda,C=o.isIterable,F=o.getFunctionArity,U=o.isDeepEqual,N=Oe(null);function L(t,r,n){var a,i,o,s;return regeneratorRuntime.wrap(function(e){for(;;)switch(e.prev=e.next){case 0:(i=n.lookup("__evaluate_entry"))&&i(t,r,n),e.t0=t.type,e.next="path"===e.t0?5:"binary"===e.t0?8:"unary"===e.t0?11:"name"===e.t0?14:"string"===e.t0?16:"number"===e.t0?16:"value"===e.t0?16:"wildcard"===e.t0?18:"descendant"===e.t0?20:"parent"===e.t0?22:"condition"===e.t0?24:"block"===e.t0?27:"bind"===e.t0?30:"regex"===e.t0?33:"function"===e.t0?35:"variable"===e.t0?38:"lambda"===e.t0?40:"partial"===e.t0?42:"apply"===e.t0?45:"transform"===e.t0?48:50;break;case 5:return e.delegateYield(I(t,r,n),"t1",6);case 6:return a=e.t1,e.abrupt("break",50);case 8:return e.delegateYield(G(t,r,n),"t2",9);case 9:return a=e.t2,e.abrupt("break",50);case 11:return e.delegateYield(H(t,r,n),"t3",12);case 12:return a=e.t3,e.abrupt("break",50);case 14:return a=Z(t,r,n),e.abrupt("break",50);case 16:return a=B(t),e.abrupt("break",50);case 18:return a=J(t,r),e.abrupt("break",50);case 20:return a=X(t,r),e.abrupt("break",50);case 22:return a=n.lookup(t.slot.label),e.abrupt("break",50);case 24:return e.delegateYield(se(t,r,n),"t4",25);case 25:return a=e.t4,e.abrupt("break",50);case 27:return e.delegateYield(ue(t,r,n),"t5",28);case 28:return a=e.t5,e.abrupt("break",50);case 30:return e.delegateYield(oe(t,r,n),"t6",31);case 31:return a=e.t6,e.abrupt("break",50);case 33:return a=ce(t),e.abrupt("break",50);case 35:return e.delegateYield(ge(t,r,n),"t7",36);case 36:return a=e.t7,e.abrupt("break",50);case 38:return a=pe(t,r,n),e.abrupt("break",50);case 40:return a=me(t,r,n),e.abrupt("break",50);case 42:return e.delegateYield(ye(t,r,n),"t8",43);case 43:return a=e.t8,e.abrupt("break",50);case 45:return e.delegateYield(he(t,r,n),"t9",46);case 46:return a=e.t9,e.abrupt("break",50);case 48:return a=le(t,r,n),e.abrupt("break",50);case 50:if(!n.async||null!=a&&"function"==typeof a.then||(a=Promise.resolve(a)),!n.async||"function"!=typeof a.then||!t.nextFunction||"function"!=typeof a[t.nextFunction]){e.next=54;break}e.next=57;break;case 54:return e.next=56,a;case 56:a=e.sent;case 57:if(!Object.prototype.hasOwnProperty.call(t,"predicate")){e.next=65;break}o=0;case 59:if(!(o<t.predicate.length)){e.next=65;break}return e.delegateYield(W(t.predicate[o].expr,a,n),"t10",61);case 61:a=e.t10;case 62:o++,e.next=59;break;case 65:if("path"===t.type||!Object.prototype.hasOwnProperty.call(t,"group")){e.next=68;break}return e.delegateYield(ne(t.group,a,n),"t11",67);case 67:a=e.t11;case 68:return(s=n.lookup("__evaluate_exit"))&&s(t,r,n,a),a&&P(a)&&!a.tupleStream&&(t.keepArray&&(a.keepSingleton=!0),0===a.length?a=void 0:1===a.length&&(a=a.keepSingleton?a:a[0])),e.abrupt("return",a);case 72:case"end":return e.stop()}},e)}function I(e,r,n){var a,i,o,s,u,c;return regeneratorRuntime.wrap(function(t){for(;;)switch(t.prev=t.next){case 0:a=Array.isArray(r)&&"variable"!==e.steps[0].type?r:Y(r),o=!1,s=void 0,u=0;case 4:if(!(u<e.steps.length)){t.next=25;break}if((c=e.steps[u]).tuple&&(o=!0),0!==u||!c.consarray){t.next=12;break}return t.delegateYield(L(c,a,n),"t0",9);case 9:i=t.t0,t.next=19;break;case 12:if(!o){t.next=17;break}return t.delegateYield(q(c,a,s,n),"t1",14);case 14:s=t.t1,t.next=19;break;case 17:return t.delegateYield($(c,a,n,u===e.steps.length-1),"t2",18);case 18:i=t.t2;case 19:if(o||void 0!==i&&0!==i.length){t.next=21;break}return t.abrupt("break",25);case 21:void 0===c.focus&&(a=i);case 22:u++,t.next=4;break;case 25:if(o)if(e.tuple)i=s;else for(i=Y(),u=0;u<s.length;u++)i.push(s[u]["@"]);if(e.keepSingletonArray&&(P(i)||(i=Y(i)),i.keepSingleton=!0),!e.hasOwnProperty("group")){t.next=30;break}return t.delegateYield(ne(e.group,o?s:i,n),"t3",29);case 29:i=t.t3;case 30:return t.abrupt("return",i);case 31:case"end":return t.stop()}},t)}function _(e,t){var r=Oe(e);for(var n in t)r.bind(n,t[n]);return r}function $(e,t,n,a){var i,o,s,u,c;return regeneratorRuntime.wrap(function(r){for(;;)switch(r.prev=r.next){case 0:if("sort"!==e.type){r.next=7;break}return r.delegateYield(fe(e,t,n),"t0",2);case 2:if(i=r.t0,!e.stages){r.next=6;break}return r.delegateYield(z(e.stages,i,n),"t1",5);case 5:i=r.t1;case 6:return r.abrupt("return",i);case 7:i=Y(),o=0;case 9:if(!(o<t.length)){r.next=24;break}return r.delegateYield(L(e,t[o],n),"t2",11);case 11:if(s=r.t2,!e.stages){r.next=20;break}u=0;case 14:if(!(u<e.stages.length)){r.next=20;break}return r.delegateYield(W(e.stages[u].expr,s,n),"t3",16);case 16:s=r.t3;case 17:u++,r.next=14;break;case 20:void 0!==s&&i.push(s);case 21:o++,r.next=9;break;case 24:return c=Y(),a&&1===i.length&&Array.isArray(i[0])&&!P(i[0])?c=i[0]:i.forEach(function(e){!Array.isArray(e)||e.cons?c.push(e):e.forEach(function(e){return c.push(e)})}),r.abrupt("return",c);case 27:case"end":return r.stop()}},r)}function z(e,t,r){var n,a,i,o;return regeneratorRuntime.wrap(function(s){for(;;)switch(s.prev=s.next){case 0:n=t,a=0;case 2:if(!(a<e.length)){s.next=15;break}i=e[a],s.t0=i.type,s.next="filter"===s.t0?7:"index"===s.t0?10:12;break;case 7:return s.delegateYield(W(i.expr,n,r),"t1",8);case 8:return n=s.t1,s.abrupt("break",12);case 10:for(o=0;o<n.length;o++)n[o][i.value]=o;return s.abrupt("break",12);case 12:a++,s.next=2;break;case 15:return s.abrupt("return",n);case 16:case"end":return s.stop()}},c)}function q(e,t,r,n){var a,i,o,s,u,c,f,l;return regeneratorRuntime.wrap(function(p){for(;;)switch(p.prev=p.next){case 0:if("sort"!==e.type){p.next=15;break}if(!r){p.next=6;break}return p.delegateYield(fe(e,r,n),"t0",3);case 3:a=p.t0,p.next=11;break;case 6:return p.delegateYield(fe(e,t,n),"t1",7);case 7:for(i=p.t1,(a=Y()).tupleStream=!0,o=0;o<i.length;o++)(s={"@":i[o]})[e.index]=o,a.push(s);case 11:if(!e.stages){p.next=14;break}return p.delegateYield(z(e.stages,a,n),"t2",13);case 13:a=p.t2;case 14:return p.abrupt("return",a);case 15:(a=Y()).tupleStream=!0,u=n,void 0===r&&(r=t.map(function(e){return{"@":e}})),c=0;case 20:if(!(c<r.length)){p.next=28;break}return u=_(n,r[c]),p.delegateYield(L(e,r[c]["@"],u),"t3",23);case 23:if(void 0!==(f=p.t3))for(Array.isArray(f)||(f=[f]),l=0;l<f.length;l++)s={},Object.assign(s,r[c]),f.tupleStream?Object.assign(s,f[l]):(e.focus?(s[e.focus]=f[l],s["@"]=r[c]["@"]):s["@"]=f[l],e.index&&(s[e.index]=l),e.ancestor&&(s[e.ancestor.label]=r[c]["@"])),a.push(s);case 25:c++,p.next=20;break;case 28:if(!e.stages){p.next=31;break}return p.delegateYield(z(e.stages,a,n),"t4",30);case 30:a=p.t4;case 31:return p.abrupt("return",a);case 32:case"end":return p.stop()}},p)}function W(e,t,r){var n,a,o,s,u,c;return regeneratorRuntime.wrap(function(p){for(;;)switch(p.prev=p.next){case 0:if(n=Y(),t&&t.tupleStream&&(n.tupleStream=!0),Array.isArray(t)||(t=Y(t)),"number"!==e.type){p.next=10;break}(a=Math.floor(e.value))<0&&(a=t.length+a),void 0!==(o=t[a])&&(Array.isArray(o)?n=o:n.push(o)),p.next=23;break;case 10:a=0;case 11:if(!(a<t.length)){p.next=23;break}return o=t[a],s=o,u=r,t.tupleStream&&(s=o["@"],u=_(r,o)),p.delegateYield(L(e,s,u),"t0",17);case 17:c=p.t0,D(c)&&(c=[c]),R(c)?c.forEach(function(e){var r=Math.floor(e);r<0&&(r=t.length+r),r===a&&n.push(o)}):i.boolean(c)&&n.push(o);case 20:a++,p.next=11;break;case 23:return p.abrupt("return",n);case 24:case"end":return p.stop()}},f)}function G(e,t,r){var n,a,i,o;return regeneratorRuntime.wrap(function(s){for(;;)switch(s.prev=s.next){case 0:return s.delegateYield(L(e.lhs,t,r),"t0",1);case 1:return a=s.t0,s.delegateYield(L(e.rhs,t,r),"t1",3);case 3:i=s.t1,o=e.value,s.prev=5,s.t2=o,s.next="+"===s.t2?9:"-"===s.t2?9:"*"===s.t2?9:"/"===s.t2?9:"%"===s.t2?9:"="===s.t2?11:"!="===s.t2?11:"<"===s.t2?13:"<="===s.t2?13:">"===s.t2?13:">="===s.t2?13:"&"===s.t2?15:"and"===s.t2?17:"or"===s.t2?17:".."===s.t2?19:"in"===s.t2?21:23;break;case 9:return n=Q(a,i,o),s.abrupt("break",23);case 11:return n=V(a,i,o),s.abrupt("break",23);case 13:return n=K(a,i,o),s.abrupt("break",23);case 15:return n=re(a,i),s.abrupt("break",23);case 17:return n=te(a,i,o),s.abrupt("break",23);case 19:return n=ie(a,i),s.abrupt("break",23);case 21:return n=ee(a,i),s.abrupt("break",23);case 23:s.next=30;break;case 25:throw s.prev=25,s.t3=s.catch(5),s.t3.position=e.position,s.t3.token=o,s.t3;case 30:return s.abrupt("return",n);case 31:case"end":return s.stop()}},l,null,[[5,25]])}function H(e,t,r){var n,a,o,s;return regeneratorRuntime.wrap(function(u){for(;;)switch(u.prev=u.next){case 0:u.t0=e.value,u.next="-"===u.t0?3:"["===u.t0?15:"{"===u.t0?27:30;break;case 3:return u.delegateYield(L(e.expression,t,r),"t1",4);case 4:if(void 0!==(n=u.t1)){u.next=9;break}n=void 0,u.next=14;break;case 9:if(!D(n)){u.next=13;break}n=-n,u.next=14;break;case 13:throw{code:"D1002",stack:(new Error).stack,position:e.position,token:e.value,value:n};case 14:return u.abrupt("break",30);case 15:n=[],a=0;case 17:if(!(a<e.expressions.length)){u.next=25;break}return o=e.expressions[a],u.delegateYield(L(o,t,r),"t2",20);case 20:void 0!==(s=u.t2)&&("["===o.value?n.push(s):n=i.append(n,s));case 22:a++,u.next=17;break;case 25:return e.consarray&&Object.defineProperty(n,"cons",{enumerable:!1,configurable:!1,value:!0}),u.abrupt("break",30);case 27:return u.delegateYield(ne(e,t,r),"t3",28);case 28:return n=u.t3,u.abrupt("break",30);case 30:return u.abrupt("return",n);case 31:case"end":return u.stop()}},d)}function Z(e,t,r){return i.lookup(t,e.value)}function B(e){return e.value}function J(e,t){var r=Y();return null!==t&&"object"===n(t)&&Object.keys(t).forEach(function(e){var n=t[e];Array.isArray(n)?(n=function e(t,r){void 0===r&&(r=[]);Array.isArray(t)?t.forEach(function(t){e(t,r)}):r.push(t);return r}(n),r=i.append(r,n)):r.push(n)}),r}function X(e,t){var r,a=Y();return void 0!==t&&(!function e(t,r){Array.isArray(t)||r.push(t);Array.isArray(t)?t.forEach(function(t){e(t,r)}):null!==t&&"object"===n(t)&&Object.keys(t).forEach(function(n){e(t[n],r)})}(t,a),r=1===a.length?a[0]:a),r}function Q(e,t,r){var n;if(void 0!==e&&!D(e))throw{code:"T2001",stack:(new Error).stack,value:e};if(void 0!==t&&!D(t))throw{code:"T2002",stack:(new Error).stack,value:t};if(void 0===e||void 0===t)return n;switch(r){case"+":n=e+t;break;case"-":n=e-t;break;case"*":n=e*t;break;case"/":n=e/t;break;case"%":n=e%t}return n}function V(e,t,r){var a,i=n(e),o=n(t);if("undefined"===i||"undefined"===o)return!1;switch(r){case"=":a=U(e,t);break;case"!=":a=!U(e,t)}return a}function K(e,t,r){var a,i=n(e),o=n(t);if(!("undefined"===i||"string"===i||"number"===i)||!("undefined"===o||"string"===o||"number"===o))throw{code:"T2010",stack:(new Error).stack,value:"string"!==i&&"number"!==i?e:t};if("undefined"!==i&&"undefined"!==o){if(i!==o)throw{code:"T2009",stack:(new Error).stack,value:e,value2:t};switch(r){case"<":a=e<t;break;case"<=":a=e<=t;break;case">":a=e>t;break;case">=":a=e>=t}return a}}function ee(e,t){var r=!1;if(void 0===e||void 0===t)return!1;Array.isArray(t)||(t=[t]);for(var n=0;n<t.length;n++)if(t[n]===e){r=!0;break}return r}function te(e,t,r){var n,a=i.boolean(e),o=i.boolean(t);switch(void 0===a&&(a=!1),void 0===o&&(o=!1),r){case"and":n=a&&o;break;case"or":n=a||o}return n}function re(e,t){var r="",n="";return void 0!==e&&(r=i.string(e)),void 0!==t&&(n=i.string(t)),r.concat(n)}function ne(e,t,r){var n,a,o,s,u,c,p,f,l,d,g,v,b;return regeneratorRuntime.wrap(function(h){for(;;)switch(h.prev=h.next){case 0:n={},a={},o=!(!t||!t.tupleStream),Array.isArray(t)||(t=Y(t)),s=0;case 5:if(!(s<t.length)){h.next=29;break}u=t[s],c=o?_(r,u):r,p=0;case 9:if(!(p<e.lhs.length)){h.next=26;break}return f=e.lhs[p],h.delegateYield(L(f[0],o?u["@"]:u,c),"t0",12);case 12:if("string"==typeof(l=h.t0)){h.next=15;break}throw{code:"T1003",stack:(new Error).stack,position:e.position,value:l};case 15:if(d={data:u,exprIndex:p},!a.hasOwnProperty(l)){h.next=22;break}if(a[l].exprIndex===p){h.next=19;break}throw{code:"D1009",stack:(new Error).stack,position:e.position,value:l};case 19:a[l].data=i.append(a[l].data,u),h.next=23;break;case 22:a[l]=d;case 23:p++,h.next=9;break;case 26:s++,h.next=5;break;case 29:h.t1=regeneratorRuntime.keys(a);case 30:if((h.t2=h.t1()).done){h.next=41;break}return l=h.t2.value,d=a[l],g=d.data,c=r,o&&(v=ae(d.data),g=v["@"],delete v["@"],c=_(r,v)),h.delegateYield(L(e.lhs[d.exprIndex][1],g,c),"t3",37);case 37:void 0!==(b=h.t3)&&(n[l]=b),h.next=30;break;case 41:return h.abrupt("return",n);case 42:case"end":return h.stop()}},h)}function ae(e){if(!Array.isArray(e))return e;var t={};Object.assign(t,e[0]);for(var r=1;r<e.length;r++)for(var n in e[r])t[n]=i.append(t[n],e[r][n]);return t}function ie(e,t){var r;if(void 0!==e&&!Number.isInteger(e))throw{code:"T2003",stack:(new Error).stack,value:e};if(void 0!==t&&!Number.isInteger(t))throw{code:"T2004",stack:(new Error).stack,value:t};if(void 0===e||void 0===t)return r;if(e>t)return r;var n=t-e+1;if(n>1e7)throw{code:"D2014",stack:(new Error).stack,value:n};r=new Array(n);for(var a=e,i=0;a<=t;a++,i++)r[i]=a;return r.sequence=!0,r}function oe(e,t,r){var n;return regeneratorRuntime.wrap(function(a){for(;;)switch(a.prev=a.next){case 0:return a.delegateYield(L(e.rhs,t,r),"t0",1);case 1:return n=a.t0,r.bind(e.lhs.value,n),a.abrupt("return",n);case 4:case"end":return a.stop()}},g)}function se(e,t,r){var n,a;return regeneratorRuntime.wrap(function(o){for(;;)switch(o.prev=o.next){case 0:return o.delegateYield(L(e.condition,t,r),"t0",1);case 1:if(a=o.t0,!i.boolean(a)){o.next=7;break}return o.delegateYield(L(e.then,t,r),"t1",4);case 4:n=o.t1,o.next=10;break;case 7:if(void 0===e.else){o.next=10;break}return o.delegateYield(L(e.else,t,r),"t2",9);case 9:n=o.t2;case 10:return o.abrupt("return",n);case 11:case"end":return o.stop()}},v)}function ue(e,t,r){var n,a,i;return regeneratorRuntime.wrap(function(o){for(;;)switch(o.prev=o.next){case 0:a=Oe(r),i=0;case 2:if(!(i<e.expressions.length)){o.next=8;break}return o.delegateYield(L(e.expressions[i],t,a),"t0",4);case 4:n=o.t0;case 5:i++,o.next=2;break;case 8:return o.abrupt("return",n);case 9:case"end":return o.stop()}},b)}function ce(e){var t=new RegExp(e.value);return function r(n,a){var i;t.lastIndex=a||0;var o=t.exec(n);if(null!==o){if(i={match:o[0],start:o.index,end:o.index+o[0].length,groups:[]},o.length>1)for(var s=1;s<o.length;s++)i.groups.push(o[s]);i.next=function(){if(!(t.lastIndex>=n.length)){var a=r(n,t.lastIndex);if(a&&""===a.match)throw{code:"D1004",stack:(new Error).stack,position:e.position,value:e.value.source};return a}}}return i}}function pe(e,t,r){return""===e.value?t&&t.outerWrapper?t[0]:t:r.lookup(e.value)}function fe(e,t,r){var a,o,s,u,c;return regeneratorRuntime.wrap(function(p){for(;;)switch(p.prev=p.next){case 0:return o=t,s=!!t.tupleStream,u=regeneratorRuntime.mark(function t(a,i){var o,u,c,p,f,l,d,h,g;return regeneratorRuntime.wrap(function(t){for(;;)switch(t.prev=t.next){case 0:o=0,u=0;case 2:if(!(0===o&&u<e.terms.length)){t.next=35;break}return c=e.terms[u],p=a,f=r,s&&(p=a["@"],f=_(r,a)),t.delegateYield(L(c.expression,p,f),"t0",8);case 8:return l=t.t0,p=i,f=r,s&&(p=i["@"],f=_(r,i)),t.delegateYield(L(c.expression,p,f),"t1",13);case 13:if(d=t.t1,h=n(l),g=n(d),"undefined"!==h){t.next=19;break}return o="undefined"===g?0:1,t.abrupt("continue",32);case 19:if("undefined"!==g){t.next=22;break}return o=-1,t.abrupt("continue",32);case 22:if(!("string"!==h&&"number"!==h||"string"!==g&&"number"!==g)){t.next=24;break}throw{code:"T2008",stack:(new Error).stack,position:e.position,value:"string"!==h&&"number"!==h?l:d};case 24:if(h===g){t.next=26;break}throw{code:"T2007",stack:(new Error).stack,position:e.position,value:l,value2:d};case 26:if(l!==d){t.next=30;break}return t.abrupt("continue",32);case 30:o=l<d?-1:1;case 31:!0===c.descending&&(o=-o);case 32:u++,t.next=2;break;case 35:return t.abrupt("return",1===o);case 36:case"end":return t.stop()}},t)}),c={environment:r,input:t},p.delegateYield(i.sort.apply(c,[o,u]),"t0",5);case 5:return a=p.t0,p.abrupt("return",a);case 7:case"end":return p.stop()}},m)}function le(e,t,r){return Te(regeneratorRuntime.mark(function t(a){var i,o,s,u,c,p,f,l,d,h,g;return regeneratorRuntime.wrap(function(t){for(;;)switch(t.prev=t.next){case 0:if(void 0!==a){t.next=2;break}return t.abrupt("return",void 0);case 2:if(i=r.lookup("clone"),M(i)){t.next=5;break}throw{code:"T2013",stack:(new Error).stack,position:e.position};case 5:return t.delegateYield(ve(i,[a],null,r),"t0",6);case 6:return o=t.t0,t.delegateYield(L(e.pattern,o,r),"t1",8);case 8:if(void 0===(s=t.t1)){t.next=33;break}Array.isArray(s)||(s=[s]),u=0;case 12:if(!(u<s.length)){t.next=33;break}return c=s[u],t.delegateYield(L(e.update,c,r),"t2",15);case 15:if(p=t.t2,"undefined"===(f=n(p))){t.next=21;break}if("object"===f&&null!==p&&!Array.isArray(p)){t.next=20;break}throw{code:"T2011",stack:(new Error).stack,position:e.update.position,value:p};case 20:for(l in p)c[l]=p[l];case 21:if(void 0===e.delete){t.next=30;break}return t.delegateYield(L(e.delete,c,r),"t3",23);case 23:if(void 0===(d=t.t3)){t.next=30;break}if(h=d,Array.isArray(d)||(d=[d]),O(d)){t.next=29;break}throw{code:"T2012",stack:(new Error).stack,position:e.delete.position,value:h};case 29:for(g=0;g<d.length;g++)"object"===n(c)&&null!==c&&delete c[d[g]];case 30:u++,t.next=12;break;case 33:return t.abrupt("return",o);case 34:case"end":return t.stop()}},t)}),"<(oa):o>")}var de=s("function($f, $g) { function($x){ $g($f($x)) } }");function he(e,t,r){var n,a,i,o;return regeneratorRuntime.wrap(function(s){for(;;)switch(s.prev=s.next){case 0:return s.delegateYield(L(e.lhs,t,r),"t0",1);case 1:if(a=s.t0,"function"!==e.rhs.type){s.next=7;break}return s.delegateYield(ge(e.rhs,t,r,{context:a}),"t1",4);case 4:n=s.t1,s.next=20;break;case 7:return s.delegateYield(L(e.rhs,t,r),"t2",8);case 8:if(i=s.t2,M(i)){s.next=11;break}throw{code:"T2006",stack:(new Error).stack,position:e.position,value:i};case 11:if(!M(a)){s.next=18;break}return s.delegateYield(L(de,null,r),"t3",13);case 13:return o=s.t3,s.delegateYield(ve(o,[a,i],null,r),"t4",15);case 15:n=s.t4,s.next=20;break;case 18:return s.delegateYield(ve(i,[a],null,r),"t5",19);case 19:n=s.t5;case 20:return s.abrupt("return",n);case 21:case"end":return s.stop()}},y)}function ge(e,t,r,a){var i,o,s,u,c,p;return regeneratorRuntime.wrap(function(f){for(;;)switch(f.prev=f.next){case 0:return f.delegateYield(L(e.procedure,t,r),"t0",1);case 1:if(void 0!==(o=f.t0)||"path"!==e.procedure.type||!r.lookup(e.procedure.steps[0].value)){f.next=4;break}throw{code:"T1005",stack:(new Error).stack,position:e.position,token:e.procedure.steps[0].value};case 4:s=[],void 0!==a&&s.push(a.context),u=regeneratorRuntime.mark(function n(){var a,i;return regeneratorRuntime.wrap(function(n){for(;;)switch(n.prev=n.next){case 0:return n.delegateYield(L(e.arguments[c],t,r),"t0",1);case 1:a=n.t0,M(a)?((i=regeneratorRuntime.mark(function e(){var t,n,i,o=arguments;return regeneratorRuntime.wrap(function(e){for(;;)switch(e.prev=e.next){case 0:for(t=o.length,n=new Array(t),i=0;i<t;i++)n[i]=o[i];return e.delegateYield(ve(a,n,null,r),"t0",2);case 2:return e.abrupt("return",e.t0);case 3:case"end":return e.stop()}},e)})).arity=F(a),s.push(i)):s.push(a);case 3:case"end":return n.stop()}},n)}),c=0;case 8:if(!(c<e.arguments.length)){f.next=13;break}return f.delegateYield(u(),"t1",10);case 10:c++,f.next=8;break;case 13:return p="path"===e.procedure.type?e.procedure.steps[0].value:e.procedure.value,f.prev=14,"object"===n(o)&&(o.token=p,o.position=e.position),f.delegateYield(ve(o,s,t,r),"t2",17);case 17:i=f.t2,f.next=25;break;case 20:throw f.prev=20,f.t3=f.catch(14),f.t3.position||(f.t3.position=e.position),f.t3.token||(f.t3.token=p),f.t3;case 25:return f.abrupt("return",i);case 26:case"end":return f.stop()}},k,null,[[14,20]])}function ve(e,t,r,n){var a,i,o,s;return regeneratorRuntime.wrap(function(u){for(;;)switch(u.prev=u.next){case 0:return u.delegateYield(be(e,t,r,n),"t0",1);case 1:a=u.t0;case 2:if(!j(a)||!0!==a.thunk){u.next=21;break}return u.delegateYield(L(a.body.procedure,a.input,a.environment),"t1",4);case 4:i=u.t1,"variable"===a.body.procedure.type&&(i.token=a.body.procedure.value),i.position=a.body.procedure.position,o=[],s=0;case 9:if(!(s<a.body.arguments.length)){u.next=17;break}return u.t2=o,u.delegateYield(L(a.body.arguments[s],a.input,a.environment),"t3",12);case 12:u.t4=u.t3,u.t2.push.call(u.t2,u.t4);case 14:s++,u.next=9;break;case 17:return u.delegateYield(be(i,o,r,n),"t5",18);case 18:a=u.t5,u.next=2;break;case 21:return u.abrupt("return",a);case 22:case"end":return u.stop()}},x)}function be(e,t,r,n){var a,i,o;return regeneratorRuntime.wrap(function(s){for(;;)switch(s.prev=s.next){case 0:if(s.prev=0,i=t,e&&(i=ke(e.signature,t,r)),!j(e)){s.next=8;break}return s.delegateYield(xe(e,i),"t0",5);case 5:a=s.t0,s.next=24;break;case 8:if(!e||!0!==e._jsonata_function){s.next=16;break}if(o={environment:n,input:r},a=e.implementation.apply(o,i),!C(a)){s.next=14;break}return s.delegateYield(a,"t1",13);case 13:a=s.t1;case 14:s.next=24;break;case 16:if("function"!=typeof e){s.next=23;break}if(a=e.apply(r,i),!C(a)){s.next=21;break}return s.delegateYield(a,"t2",20);case 20:a=s.t2;case 21:s.next=24;break;case 23:throw{code:"T1006",stack:(new Error).stack};case 24:s.next=30;break;case 26:throw s.prev=26,s.t3=s.catch(0),e&&(void 0===s.t3.token&&void 0!==e.token&&(s.t3.token=e.token),s.t3.position=e.position),s.t3;case 30:return s.abrupt("return",a);case 31:case"end":return s.stop()}},w,null,[[0,26]])}function me(e,t,r){var n={_jsonata_lambda:!0,input:t,environment:r,arguments:e.arguments,signature:e.signature,body:e.body};return!0===e.thunk&&(n.thunk=!0),n.apply=regeneratorRuntime.mark(function e(r,a){return regeneratorRuntime.wrap(function(e){for(;;)switch(e.prev=e.next){case 0:return e.delegateYield(ve(n,a,t,r.environment),"t0",1);case 1:return e.abrupt("return",e.t0);case 2:case"end":return e.stop()}},e)}),n}function ye(e,t,r){var n,a,i,o,s;return regeneratorRuntime.wrap(function(u){for(;;)switch(u.prev=u.next){case 0:a=[],i=0;case 2:if(!(i<e.arguments.length)){u.next=15;break}if("operator"!==(o=e.arguments[i]).type||"?"!==o.value){u.next=8;break}a.push(o),u.next=12;break;case 8:return u.t0=a,u.delegateYield(L(o,t,r),"t1",10);case 10:u.t2=u.t1,u.t0.push.call(u.t0,u.t2);case 12:i++,u.next=2;break;case 15:return u.delegateYield(L(e.procedure,t,r),"t3",16);case 16:if(void 0!==(s=u.t3)||"path"!==e.procedure.type||!r.lookup(e.procedure.steps[0].value)){u.next=19;break}throw{code:"T1007",stack:(new Error).stack,position:e.position,token:e.procedure.steps[0].value};case 19:if(!j(s)){u.next=23;break}n=we(s,a),u.next=32;break;case 23:if(!s||!0!==s._jsonata_function){u.next=27;break}n=Ee(s.implementation,a),u.next=32;break;case 27:if("function"!=typeof s){u.next=31;break}n=Ee(s,a),u.next=32;break;case 31:throw{code:"T1008",stack:(new Error).stack,position:e.position,token:"path"===e.procedure.type?e.procedure.steps[0].value:e.procedure.value};case 32:return u.abrupt("return",n);case 33:case"end":return u.stop()}},E)}function ke(e,t,r){return void 0===e?t:e.validate(t,r)}function xe(e,t){var r,n;return regeneratorRuntime.wrap(function(a){for(;;)switch(a.prev=a.next){case 0:if(n=Oe(e.environment),e.arguments.forEach(function(e,r){n.bind(e.value,t[r])}),"function"!=typeof e.body){a.next=7;break}return a.delegateYield(Ae(e.body,n),"t0",4);case 4:r=a.t0,a.next=9;break;case 7:return a.delegateYield(L(e.body,e.input,n),"t1",8);case 8:r=a.t1;case 9:return a.abrupt("return",r);case 10:case"end":return a.stop()}},A)}function we(e,t){var r=Oe(e.environment),n=[];return e.arguments.forEach(function(e,a){var i=t[a];i&&"operator"===i.type&&"?"===i.value?n.push(e):r.bind(e.value,i)}),{_jsonata_lambda:!0,input:e.input,environment:r,arguments:n,body:e.body}}function Ee(e,t){var r=Se(e),n="function("+(r=r.map(function(e){return"$"+e.trim()})).join(", ")+"){ _ }",a=s(n);return a.body=e,we(a,t)}function Ae(e,t){var r,n,a,i;return regeneratorRuntime.wrap(function(o){for(;;)switch(o.prev=o.next){case 0:if(r=Se(e),n=r.map(function(e){return t.lookup(e.trim())}),a={environment:t},i=e.apply(a,n),!C(i)){o.next=7;break}return o.delegateYield(i,"t0",6);case 6:i=o.t0;case 7:return o.abrupt("return",i);case 8:case"end":return o.stop()}},S)}function Se(e){var t=e.toString();return/\(([^)]*)\)/.exec(t)[1].split(",")}function Te(e,t){var r={_jsonata_function:!0,implementation:e};return void 0!==t&&(r.signature=u(t)),r}function De(e,t){var r,n,a;return regeneratorRuntime.wrap(function(i){for(;;)switch(i.prev=i.next){case 0:if(void 0!==e){i.next=2;break}return i.abrupt("return",void 0);case 2:r=this.input,void 0!==t&&(r=t),i.prev=4,n=s(e,!1),i.next=12;break;case 8:throw i.prev=8,i.t0=i.catch(4),Ye(i.t0),{stack:(new Error).stack,code:"D3120",value:i.t0.message,error:i.t0};case 12:return i.prev=12,i.delegateYield(L(n,r,this.environment),"t1",14);case 14:a=i.t1,i.next=21;break;case 17:throw i.prev=17,i.t2=i.catch(12),Ye(i.t2),{stack:(new Error).stack,code:"D3121",value:i.t2.message,error:i.t2};case 21:return i.abrupt("return",a);case 22:case"end":return i.stop()}},T,this,[[4,8],[12,17]])}function Oe(e){var t={};return{bind:function(e,r){t[e]=r},lookup:function(r){var n;return t.hasOwnProperty(r)?n=t[r]:e&&(n=e.lookup(r)),n},timestamp:e?e.timestamp:null,async:!!e&&e.async,global:e?e.global:{ancestry:[null]}}}N.bind("sum",Te(i.sum,"<a<n>:n>")),N.bind("count",Te(i.count,"<a:n>")),N.bind("max",Te(i.max,"<a<n>:n>")),N.bind("min",Te(i.min,"<a<n>:n>")),N.bind("average",Te(i.average,"<a<n>:n>")),N.bind("string",Te(i.string,"<x-b?:s>")),N.bind("substring",Te(i.substring,"<s-nn?:s>")),N.bind("substringBefore",Te(i.substringBefore,"<s-s:s>")),N.bind("substringAfter",Te(i.substringAfter,"<s-s:s>")),N.bind("lowercase",Te(i.lowercase,"<s-:s>")),N.bind("uppercase",Te(i.uppercase,"<s-:s>")),N.bind("length",Te(i.length,"<s-:n>")),N.bind("trim",Te(i.trim,"<s-:s>")),N.bind("pad",Te(i.pad,"<s-ns?:s>")),N.bind("match",Te(i.match,"<s-f<s:o>n?:a<o>>")),N.bind("contains",Te(i.contains,"<s-(sf):b>")),N.bind("replace",Te(i.replace,"<s-(sf)(sf)n?:s>")),N.bind("split",Te(i.split,"<s-(sf)n?:a<s>>")),N.bind("join",Te(i.join,"<a<s>s?:s>")),N.bind("formatNumber",Te(i.formatNumber,"<n-so?:s>")),N.bind("formatBase",Te(i.formatBase,"<n-n?:s>")),N.bind("formatInteger",Te(a.formatInteger,"<n-s:s>")),N.bind("parseInteger",Te(a.parseInteger,"<s-s:n>")),N.bind("number",Te(i.number,"<(nsb)-:n>")),N.bind("floor",Te(i.floor,"<n-:n>")),N.bind("ceil",Te(i.ceil,"<n-:n>")),N.bind("round",Te(i.round,"<n-n?:n>")),N.bind("abs",Te(i.abs,"<n-:n>")),N.bind("sqrt",Te(i.sqrt,"<n-:n>")),N.bind("power",Te(i.power,"<n-n:n>")),N.bind("random",Te(i.random,"<:n>")),N.bind("boolean",Te(i.boolean,"<x-:b>")),N.bind("not",Te(i.not,"<x-:b>")),N.bind("map",Te(i.map,"<af>")),N.bind("zip",Te(i.zip,"<a+>")),N.bind("filter",Te(i.filter,"<af>")),N.bind("single",Te(i.single,"<af?>")),N.bind("reduce",Te(i.foldLeft,"<afj?:j>")),N.bind("sift",Te(i.sift,"<o-f?:o>")),N.bind("keys",Te(i.keys,"<x-:a<s>>")),N.bind("lookup",Te(i.lookup,"<x-s:x>")),N.bind("append",Te(i.append,"<xx:a>")),N.bind("exists",Te(i.exists,"<x:b>")),N.bind("spread",Te(i.spread,"<x-:a<o>>")),N.bind("merge",Te(i.merge,"<a<o>:o>")),N.bind("reverse",Te(i.reverse,"<a:a>")),N.bind("each",Te(i.each,"<o-f:a>")),N.bind("error",Te(i.error,"<s?:x>")),N.bind("assert",Te(i.assert,"<bs?:x>")),N.bind("type",Te(i.type,"<x:s>")),N.bind("sort",Te(i.sort,"<af?:a>")),N.bind("shuffle",Te(i.shuffle,"<a:a>")),N.bind("distinct",Te(i.distinct,"<x:x>")),N.bind("base64encode",Te(i.base64encode,"<s-:s>")),N.bind("base64decode",Te(i.base64decode,"<s-:s>")),N.bind("encodeUrlComponent",Te(i.encodeUrlComponent,"<s-:s>")),N.bind("encodeUrl",Te(i.encodeUrl,"<s-:s>")),N.bind("decodeUrlComponent",Te(i.decodeUrlComponent,"<s-:s>")),N.bind("decodeUrl",Te(i.decodeUrl,"<s-:s>")),N.bind("eval",Te(De,"<sx?:x>")),N.bind("toMillis",Te(a.toMillis,"<s-s?:n>")),N.bind("fromMillis",Te(a.fromMillis,"<n-s?s?:s>")),N.bind("clone",Te(function(e){if(void 0!==e)return JSON.parse(i.string(e))},"<(oa)-:o>"));var Re={S0101:"String literal must be terminated by a matching quote",S0102:"Number out of range: {{token}}",S0103:"Unsupported escape sequence: \\{{token}}",S0104:"The escape sequence \\u must be followed by 4 hex digits",S0105:"Quoted property name must be terminated with a backquote (`)",S0106:"Comment has no closing tag",S0201:"Syntax error: {{token}}",S0202:"Expected {{value}}, got {{token}}",S0203:"Expected {{value}} before end of expression",S0204:"Unknown operator: {{token}}",S0205:"Unexpected token: {{token}}",S0206:"Unknown expression type: {{token}}",S0207:"Unexpected end of expression",S0208:"Parameter {{value}} of function definition must be a variable name (start with $)",S0209:"A predicate cannot follow a grouping expression in a step",S0210:"Each step can only have one grouping expression",S0211:"The symbol {{token}} cannot be used as a unary operator",S0212:"The left side of := must be a variable name (start with $)",S0213:"The literal value {{value}} cannot be used as a step within a path expression",S0214:"The right side of {{token}} must be a variable name (start with $)",S0215:"A context variable binding must precede any predicates on a step",S0216:"A context variable binding must precede the 'order-by' clause on a step",S0217:"The object representing the 'parent' cannot be derived from this expression",S0301:"Empty regular expressions are not allowed",S0302:"No terminating / in regular expression",S0402:"Choice groups containing parameterized types are not supported",S0401:"Type parameters can only be applied to functions and arrays",S0500:"Attempted to evaluate an expression containing syntax error(s)",T0410:"Argument {{index}} of function {{token}} does not match function signature",T0411:"Context value is not a compatible type with argument {{index}} of function {{token}}",T0412:"Argument {{index}} of function {{token}} must be an array of {{type}}",D1001:"Number out of range: {{value}}",D1002:"Cannot negate a non-numeric value: {{value}}",T1003:"Key in object structure must evaluate to a string; got: {{value}}",D1004:"Regular expression matches zero length string",T1005:"Attempted to invoke a non-function. Did you mean ${{{token}}}?",T1006:"Attempted to invoke a non-function",T1007:"Attempted to partially apply a non-function. Did you mean ${{{token}}}?",T1008:"Attempted to partially apply a non-function",D1009:"Multiple key definitions evaluate to same key: {{value}}",T1010:"The matcher function argument passed to function {{token}} does not return the correct object structure",T2001:"The left side of the {{token}} operator must evaluate to a number",T2002:"The right side of the {{token}} operator must evaluate to a number",T2003:"The left side of the range operator (..) must evaluate to an integer",T2004:"The right side of the range operator (..) must evaluate to an integer",D2005:"The left side of := must be a variable name (start with $)",T2006:"The right side of the function application operator ~> must be a function",T2007:"Type mismatch when comparing values {{value}} and {{value2}} in order-by clause",T2008:"The expressions within an order-by clause must evaluate to numeric or string values",T2009:"The values {{value}} and {{value2}} either side of operator {{token}} must be of the same data type",T2010:"The expressions either side of operator {{token}} must evaluate to numeric or string values",T2011:"The insert/update clause of the transform expression must evaluate to an object: {{value}}",T2012:"The delete clause of the transform expression must evaluate to a string or array of strings: {{value}}",T2013:"The transform expression clones the input object using the $clone() function.  This has been overridden in the current scope by a non-function.",D2014:"The size of the sequence allocated by the range operator (..) must not exceed 1e6.  Attempted to allocate {{value}}.",D3001:"Attempting to invoke string function on Infinity or NaN",D3010:"Second argument of replace function cannot be an empty string",D3011:"Fourth argument of replace function must evaluate to a positive number",D3012:"Attempted to replace a matched string with a non-string value",D3020:"Third argument of split function must evaluate to a positive number",D3030:"Unable to cast value to a number: {{value}}",D3040:"Third argument of match function must evaluate to a positive number",D3050:"The second argument of reduce function must be a function with at least two arguments",D3060:"The sqrt function cannot be applied to a negative number: {{value}}",D3061:"The power function has resulted in a value that cannot be represented as a JSON number: base={{value}}, exponent={{exp}}",D3070:"The single argument form of the sort function can only be applied to an array of strings or an array of numbers.  Use the second argument to specify a comparison function",D3080:"The picture string must only contain a maximum of two sub-pictures",D3081:"The sub-picture must not contain more than one instance of the 'decimal-separator' character",D3082:"The sub-picture must not contain more than one instance of the 'percent' character",D3083:"The sub-picture must not contain more than one instance of the 'per-mille' character",D3084:"The sub-picture must not contain both a 'percent' and a 'per-mille' character",D3085:"The mantissa part of a sub-picture must contain at least one character that is either an 'optional digit character' or a member of the 'decimal digit family'",D3086:"The sub-picture must not contain a passive character that is preceded by an active character and that is followed by another active character",D3087:"The sub-picture must not contain a 'grouping-separator' character that appears adjacent to a 'decimal-separator' character",D3088:"The sub-picture must not contain a 'grouping-separator' at the end of the integer part",D3089:"The sub-picture must not contain two adjacent instances of the 'grouping-separator' character",D3090:"The integer part of the sub-picture must not contain a member of the 'decimal digit family' that is followed by an instance of the 'optional digit character'",D3091:"The fractional part of the sub-picture must not contain an instance of the 'optional digit character' that is followed by a member of the 'decimal digit family'",D3092:"A sub-picture that contains a 'percent' or 'per-mille' character must not contain a character treated as an 'exponent-separator'",D3093:"The exponent part of the sub-picture must comprise only of one or more characters that are members of the 'decimal digit family'",D3100:"The radix of the formatBase function must be between 2 and 36.  It was given {{value}}",D3110:"The argument of the toMillis function must be an ISO 8601 formatted timestamp. Given {{value}}",D3120:"Syntax error in expression passed to function eval: {{value}}",D3121:"Dynamic error evaluating the expression passed to function eval: {{value}}",D3130:"Formatting or parsing an integer as a sequence starting with {{value}} is not supported by this implementation",D3131:"In a decimal digit pattern, all digits must be from the same decimal group",D3132:"Unknown component specifier {{value}} in date/time picture string",D3133:"The 'name' modifier can only be applied to months and days in the date/time picture string, not {{value}}",D3134:"The timezone integer format specifier cannot have more than four digits",D3135:"No matching closing bracket ']' in date/time picture string",D3136:"The date/time picture string is missing specifiers required to parse the timestamp",D3137:"{{{message}}}",D3138:"The $single() function expected exactly 1 matching result.  Instead it matched more.",D3139:"The $single() function expected exactly 1 matching result.  Instead it matched 0.",D3140:"Malformed URL passed to ${{{functionName}}}(): {{value}}",D3141:"{{{message}}}"};function Ye(e){var t=Re[e.code];if(void 0!==t){var r=t.replace(/\{\{\{([^}]+)}}}/g,function(){return e[arguments[1]]});r=r.replace(/\{\{([^}]+)}}/g,function(){return JSON.stringify(e[arguments[1]])}),e.message=r}}function Pe(e,t){var r,n;try{r=s(e,t&&t.recover),n=r.errors,delete r.errors}catch(e){throw Ye(e),e}var i=Oe(N),o=new Date;return i.bind("now",Te(function(e,t){return a.fromMillis(o.getTime(),e,t)},"<s?s?:s>")),i.bind("millis",Te(function(){return o.getTime()},"<:n>")),{evaluate:function(e,t,a){if(void 0!==n){var s={code:"S0500",position:0};throw Ye(s),s}var u,c,p;if(void 0!==t)for(var f in u=Oe(i),t)u.bind(f,t[f]);else u=i;if(u.bind("$",e),o=new Date,u.timestamp=o,Array.isArray(e)&&!P(e)&&((e=Y(e)).outerWrapper=!0),"function"==typeof a){u.async=!0;var l=function(e){Ye(e),a(e,null)};p=L(r,e,u),(c=p.next()).value.then(function e(t){(c=p.next(t)).done?a(null,c.value):c.value.then(e).catch(l)}).catch(l)}else try{for(p=L(r,e,u),c=p.next();!c.done;)c=p.next(c.value);return c.value}catch(s){throw Ye(s),s}},assign:function(e,t){i.bind(e,t)},registerFunction:function(e,t,r){var n=Te(t,r);i.bind(e,n)},ast:function(){return r},errors:function(){return n}}}return Pe.parser=s,Pe}();t.exports=c},{"./datetime":1,"./functions":2,"./parser":4,"./signature":5,"./utils":6}],4:[function(e,t,r){"use strict";var n,a,i,o=e("./signature"),s=(n={".":75,"[":80,"]":0,"{":70,"}":0,"(":80,")":0,",":0,"@":80,"#":80,";":80,":":80,"?":20,"+":50,"-":50,"*":60,"/":60,"%":60,"|":20,"=":40,"<":40,">":40,"^":40,"**":60,"..":20,":=":10,"!=":40,"<=":40,">=":40,"~>":40,and:30,or:25,in:40,"&":50,"!":0,"~":0},a={'"':'"',"\\":"\\","/":"/",b:"\b",f:"\f",n:"\n",r:"\r",t:"\t"},i=function(e){var t=0,r=e.length,i=function(e,r){return{type:e,value:r,position:t}};return function o(s){if(t>=r)return null;for(var u=e.charAt(t);t<r&&" \t\n\r\v".indexOf(u)>-1;)t++,u=e.charAt(t);if("/"===u&&"*"===e.charAt(t+1)){var c=t;for(t+=2,u=e.charAt(t);"*"!==u||"/"!==e.charAt(t+1);)if(u=e.charAt(++t),t>=r)throw{code:"S0106",stack:(new Error).stack,position:c};return t+=2,u=e.charAt(t),o(s)}if(!0!==s&&"/"===u)return t++,i("regex",function(){for(var n,a,i=t,o=0;t<r;){var s=e.charAt(t);if("/"===s&&"\\"!==e.charAt(t-1)&&0===o){if(""===(n=e.substring(i,t)))throw{code:"S0301",stack:(new Error).stack,position:t};for(t++,s=e.charAt(t),i=t;"i"===s||"m"===s;)t++,s=e.charAt(t);return a=e.substring(i,t)+"g",new RegExp(n,a)}"("!==s&&"["!==s&&"{"!==s||"\\"===e.charAt(t-1)||o++,")"!==s&&"]"!==s&&"}"!==s||"\\"===e.charAt(t-1)||o--,t++}throw{code:"S0302",stack:(new Error).stack,position:t}}());if("."===u&&"."===e.charAt(t+1))return t+=2,i("operator","..");if(":"===u&&"="===e.charAt(t+1))return t+=2,i("operator",":=");if("!"===u&&"="===e.charAt(t+1))return t+=2,i("operator","!=");if(">"===u&&"="===e.charAt(t+1))return t+=2,i("operator",">=");if("<"===u&&"="===e.charAt(t+1))return t+=2,i("operator","<=");if("*"===u&&"*"===e.charAt(t+1))return t+=2,i("operator","**");if("~"===u&&">"===e.charAt(t+1))return t+=2,i("operator","~>");if(Object.prototype.hasOwnProperty.call(n,u))return t++,i("operator",u);if('"'===u||"'"===u){var p=u;t++;for(var f="";t<r;){if("\\"===(u=e.charAt(t)))if(t++,u=e.charAt(t),Object.prototype.hasOwnProperty.call(a,u))f+=a[u];else{if("u"!==u)throw{code:"S0103",stack:(new Error).stack,position:t,token:u};var l=e.substr(t+1,4);if(!/^[0-9a-fA-F]+$/.test(l))throw{code:"S0104",stack:(new Error).stack,position:t};var d=parseInt(l,16);f+=String.fromCharCode(d),t+=4}else{if(u===p)return t++,i("string",f);f+=u}t++}throw{code:"S0101",stack:(new Error).stack,position:t}}var h,g=/^-?(0|([1-9][0-9]*))(\.[0-9]+)?([Ee][-+]?[0-9]+)?/.exec(e.substring(t));if(null!==g){var v=parseFloat(g[0]);if(!isNaN(v)&&isFinite(v))return t+=g[0].length,i("number",v);throw{code:"S0102",stack:(new Error).stack,position:t,token:g[0]}}if("`"===u){t++;var b=e.indexOf("`",t);if(-1!==b)return h=e.substring(t,b),t=b+1,i("name",h);throw t=r,{code:"S0105",stack:(new Error).stack,position:t}}for(var m,y=t;;)if(m=e.charAt(y),y===r||" \t\n\r\v".indexOf(m)>-1||Object.prototype.hasOwnProperty.call(n,m)){if("$"===e.charAt(t))return h=e.substring(t+1,y),t=y,i("variable",h);switch(h=e.substring(t,y),t=y,h){case"or":case"in":case"and":return i("operator",h);case"true":return i("value",!0);case"false":return i("value",!1);case"null":return i("value",null);default:return t===r&&""===h?null:i("name",h)}}else y++}},function(e,t){var r,a,s={},u=[],c=function(){var e=[];"(end)"!==r.id&&e.push({type:r.type,value:r.value,position:r.position});for(var t=a();null!==t;)e.push(t),t=a();return e},p={nud:function(){var e={code:"S0211",token:this.value,position:this.position};if(t)return e.remaining=c(),e.type="error",u.push(e),e;throw e.stack=(new Error).stack,e}},f=function(e,t){var r=s[e];return t=t||0,r?t>=r.lbp&&(r.lbp=t):((r=Object.create(p)).id=r.value=e,r.lbp=t,s[e]=r),r},l=function(e){if(t){e.remaining=c(),u.push(e);var n=s["(error)"];return(r=Object.create(n)).error=e,r.type="(error)",r}throw e.stack=(new Error).stack,e},d=function(t,n){if(t&&r.id!==t){var i={code:"(end)"===r.id?"S0203":"S0202",position:r.position,token:r.value,value:t};return l(i)}var o=a(n);if(null===o)return(r=s["(end)"]).position=e.length,r;var u,c=o.value,p=o.type;switch(p){case"name":case"variable":u=s["(name)"];break;case"operator":if(!(u=s[c]))return l({code:"S0204",stack:(new Error).stack,position:o.position,token:c});break;case"string":case"number":case"value":u=s["(literal)"];break;case"regex":p="regex",u=s["(regex)"];break;default:return l({code:"S0205",stack:(new Error).stack,position:o.position,token:c})}return(r=Object.create(u)).value=c,r.type=p,r.position=o.position,r},h=function(e){var t,n=r;for(d(null,!0),t=n.nud();e<r.lbp;)n=r,d(),t=n.led(t);return t},g=function(e){f(e,0).nud=function(){return this}},v=function(e,t,r){var a=t||n[e],i=f(e,a);return i.led=r||function(e){return this.lhs=e,this.rhs=h(a),this.type="binary",this},i},b=function(e,t,r){var n=f(e,t);return n.led=r,n},m=function(e,t){var r=f(e);return r.nud=t||function(){return this.expression=h(70),this.type="unary",this},r};g("(end)"),g("(name)"),g("(literal)"),g("(regex)"),f(":"),f(";"),f(","),f(")"),f("]"),f("}"),f(".."),v("."),v("+"),v("-"),v("*"),v("/"),v("%"),v("="),v("<"),v(">"),v("!="),v("<="),v(">="),v("&"),v("and"),v("or"),v("in"),g("and"),g("or"),g("in"),m("-"),v("~>"),b("(error)",10,function(e){return this.lhs=e,this.error=r.error,this.remaining=c(),this.type="error",this}),m("*",function(){return this.type="wildcard",this}),m("**",function(){return this.type="descendant",this}),m("%",function(){return this.type="parent",this}),v("(",n["("],function(e){if(this.procedure=e,this.type="function",this.arguments=[],")"!==r.id)for(;"operator"===r.type&&"?"===r.id?(this.type="partial",this.arguments.push(r),d("?")):this.arguments.push(h(0)),","===r.id;)d(",");if(d(")",!0),"name"===e.type&&("function"===e.value||"λ"===e.value)){if(this.arguments.forEach(function(e,t){if("variable"!==e.type)return l({code:"S0208",stack:(new Error).stack,position:e.position,token:e.value,value:t+1})}),this.type="lambda","<"===r.id){for(var t=r.position,n=1,a="<";n>0&&"{"!==r.id&&"(end)"!==r.id;){var i=d();">"===i.id?n--:"<"===i.id&&n++,a+=i.value}d(">");try{this.signature=o(a)}catch(e){return e.position=t+e.offset,l(e)}}d("{"),this.body=h(0),d("}")}return this}),m("(",function(){for(var e=[];")"!==r.id&&(e.push(h(0)),";"===r.id);)d(";");return d(")",!0),this.type="block",this.expressions=e,this}),m("[",function(){var e=[];if("]"!==r.id)for(;;){var t=h(0);if(".."===r.id){var n={type:"binary",value:"..",position:r.position,lhs:t};d(".."),n.rhs=h(0),t=n}if(e.push(t),","!==r.id)break;d(",")}return d("]",!0),this.expressions=e,this.type="unary",this}),v("[",n["["],function(e){if("]"===r.id){for(var t=e;t&&"binary"===t.type&&"["===t.value;)t=t.lhs;return t.keepArray=!0,d("]"),e}return this.lhs=e,this.rhs=h(n["]"]),this.type="binary",d("]",!0),this}),v("^",n["^"],function(e){d("(");for(var t=[];;){var n={descending:!1};if("<"===r.id?d("<"):">"===r.id&&(n.descending=!0,d(">")),n.expression=h(0),t.push(n),","!==r.id)break;d(",")}return d(")"),this.lhs=e,this.rhs=t,this.type="binary",this});var y=function(e){var t=[];if("}"!==r.id)for(;;){var n=h(0);d(":");var a=h(0);if(t.push([n,a]),","!==r.id)break;d(",")}return d("}",!0),void 0===e?(this.lhs=t,this.type="unary"):(this.lhs=e,this.rhs=t,this.type="binary"),this};m("{",y),v("{",n["{"],y),b(":=",n[":="],function(e){return"variable"!==e.type?l({code:"S0212",stack:(new Error).stack,position:e.position,token:e.value}):(this.lhs=e,this.rhs=h(n[":="]-1),this.type="binary",this)}),v("@",n["@"],function(e){return this.lhs=e,this.rhs=h(n["@"]),"variable"!==this.rhs.type?l({code:"S0214",stack:(new Error).stack,position:this.rhs.position,token:"@"}):(this.type="binary",this)}),v("#",n["#"],function(e){return this.lhs=e,this.rhs=h(n["#"]),"variable"!==this.rhs.type?l({code:"S0214",stack:(new Error).stack,position:this.rhs.position,token:"#"}):(this.type="binary",this)}),v("?",n["?"],function(e){return this.type="condition",this.condition=e,this.then=h(0),":"===r.id&&(d(":"),this.else=h(0)),this}),m("|",function(){return this.type="transform",this.pattern=h(0),d("|"),this.update=h(0),","===r.id&&(d(","),this.delete=h(0)),d("|"),this});var k=0,x=0,w=[],E=function e(t,r){switch(t.type){case"name":case"wildcard":r.level--,0===r.level&&(void 0===t.ancestor?t.ancestor=r:(w[r.index].slot.label=t.ancestor.label,t.ancestor=r),t.tuple=!0);break;case"parent":r.level++;break;case"block":t.expressions.length>0&&(t.tuple=!0,r=e(t.expressions[t.expressions.length-1],r));break;case"path":t.tuple=!0;var n=t.steps.length-1;for(r=e(t.steps[n--],r);r.level>0&&n>=0;)r=e(t.steps[n--],r);break;default:throw{code:"S0217",token:t.type,position:t.position}}return r},A=function(e,t){if(void 0!==t.seekingParent||"parent"===t.type){var r=void 0!==t.seekingParent?t.seekingParent:[];"parent"===t.type&&r.push(t.slot),void 0===e.seekingParent?e.seekingParent=r:Array.prototype.push.apply(e.seekingParent,r)}},S=function(e){var t=e.steps.length-1,r=e.steps[t],n=void 0!==r.seekingParent?r.seekingParent:[];"parent"===r.type&&n.push(r.slot);for(var a=0;a<n.length;a++){var i=n[a];for(t=e.steps.length-2;i.level>0;){if(t<0){void 0===e.seekingParent?e.seekingParent=[i]:e.seekingParent.push(i);break}for(var o=e.steps[t--];t>=0&&o.focus&&e.steps[t].focus;)o=e.steps[t--];i=E(o,i)}}};a=i(e),d();var T=h(0);if("(end)"!==r.id){var D={code:"S0201",position:r.position,token:r.value};l(D)}if("parent"===(T=function e(r){var n;switch(r.type){case"binary":switch(r.value){case".":var a=e(r.lhs);n="path"===a.type?a:{type:"path",steps:[a]},"parent"===a.type&&(n.seekingParent=[a.slot]);var i=e(r.rhs);"function"===i.type&&"path"===i.procedure.type&&1===i.procedure.steps.length&&"name"===i.procedure.steps[0].type&&"function"===n.steps[n.steps.length-1].type&&(n.steps[n.steps.length-1].nextFunction=i.procedure.steps[0].value),"path"===i.type?Array.prototype.push.apply(n.steps,i.steps):(void 0!==i.predicate&&(i.stages=i.predicate,delete i.predicate),n.steps.push(i)),n.steps.filter(function(e){if("number"===e.type||"value"===e.type)throw{code:"S0213",stack:(new Error).stack,position:e.position,value:e.value};return"string"===e.type}).forEach(function(e){e.type="name"}),n.steps.filter(function(e){return!0===e.keepArray}).length>0&&(n.keepSingletonArray=!0);var o=n.steps[0];"unary"===o.type&&"["===o.value&&(o.consarray=!0);var s=n.steps[n.steps.length-1];"unary"===s.type&&"["===s.value&&(s.consarray=!0),S(n);break;case"[":var c=n=e(r.lhs),p="predicate";if("path"===n.type&&(c=n.steps[n.steps.length-1],p="stages"),void 0!==c.group)throw{code:"S0209",stack:(new Error).stack,position:r.position};void 0===c[p]&&(c[p]=[]);var f=e(r.rhs);void 0!==f.seekingParent&&(f.seekingParent.forEach(function(e){1===e.level?E(c,e):e.level--}),A(c,f)),c[p].push({type:"filter",expr:f,position:r.position});break;case"{":if(void 0!==(n=e(r.lhs)).group)throw{code:"S0210",stack:(new Error).stack,position:r.position};n.group={lhs:r.rhs.map(function(t){return[e(t[0]),e(t[1])]}),position:r.position};break;case"^":"path"!==(n=e(r.lhs)).type&&(n={type:"path",steps:[n]});var l={type:"sort",position:r.position};l.terms=r.rhs.map(function(t){var r=e(t.expression);return A(l,r),{descending:t.descending,expression:r}}),n.steps.push(l),S(n);break;case":=":(n={type:"bind",value:r.value,position:r.position}).lhs=e(r.lhs),n.rhs=e(r.rhs),A(n,n.rhs);break;case"@":if(n=e(r.lhs),c=n,"path"===n.type&&(c=n.steps[n.steps.length-1]),void 0!==c.stages||void 0!==c.predicate)throw{code:"S0215",stack:(new Error).stack,position:r.position};if("sort"===c.type)throw{code:"S0216",stack:(new Error).stack,position:r.position};r.keepArray&&(c.keepArray=!0),c.focus=r.rhs.value,c.tuple=!0;break;case"#":n=e(r.lhs),c=n,"path"===n.type?c=n.steps[n.steps.length-1]:(n={type:"path",steps:[n]},void 0!==c.predicate&&(c.stages=c.predicate,delete c.predicate)),void 0===c.stages?c.index=r.rhs.value:c.stages.push({type:"index",value:r.rhs.value,position:r.position}),c.tuple=!0;break;case"~>":(n={type:"apply",value:r.value,position:r.position}).lhs=e(r.lhs),n.rhs=e(r.rhs);break;default:(n={type:r.type,value:r.value,position:r.position}).lhs=e(r.lhs),n.rhs=e(r.rhs),A(n,n.lhs),A(n,n.rhs)}break;case"unary":n={type:r.type,value:r.value,position:r.position},"["===r.value?n.expressions=r.expressions.map(function(t){var r=e(t);return A(n,r),r}):"{"===r.value?n.lhs=r.lhs.map(function(t){var r=e(t[0]);A(n,r);var a=e(t[1]);return A(n,a),[r,a]}):(n.expression=e(r.expression),"-"===r.value&&"number"===n.expression.type?(n=n.expression).value=-n.value:A(n,n.expression));break;case"function":case"partial":(n={type:r.type,name:r.name,value:r.value,position:r.position}).arguments=r.arguments.map(function(t){var r=e(t);return A(n,r),r}),n.procedure=e(r.procedure);break;case"lambda":n={type:r.type,arguments:r.arguments,signature:r.signature,position:r.position};var d=e(r.body);n.body=function e(t){var r;if("function"!==t.type||t.predicate)if("condition"===t.type)t.then=e(t.then),void 0!==t.else&&(t.else=e(t.else)),r=t;else if("block"===t.type){var n=t.expressions.length;n>0&&(t.expressions[n-1]=e(t.expressions[n-1])),r=t}else r=t;else{var a={type:"lambda",thunk:!0,arguments:[],position:t.position};a.body=t,r=a}return r}(d);break;case"condition":(n={type:r.type,position:r.position}).condition=e(r.condition),A(n,n.condition),n.then=e(r.then),A(n,n.then),void 0!==r.else&&(n.else=e(r.else),A(n,n.else));break;case"transform":(n={type:r.type,position:r.position}).pattern=e(r.pattern),n.update=e(r.update),void 0!==r.delete&&(n.delete=e(r.delete));break;case"block":(n={type:r.type,position:r.position}).expressions=r.expressions.map(function(t){var r=e(t);return A(n,r),(r.consarray||"path"===r.type&&r.steps[0].consarray)&&(n.consarray=!0),r});break;case"name":n={type:"path",steps:[r]},r.keepArray&&(n.keepSingletonArray=!0);break;case"parent":n={type:"parent",slot:{label:"!"+k++,level:1,index:x++}},w.push(n);break;case"string":case"number":case"value":case"wildcard":case"descendant":case"variable":case"regex":n=r;break;case"operator":if("and"===r.value||"or"===r.value||"in"===r.value)r.type="name",n=e(r);else{if("?"!==r.value)throw{code:"S0201",stack:(new Error).stack,position:r.position,token:r.value};n=r}break;case"error":n=r,r.lhs&&(n=e(r.lhs));break;default:var h="S0206";"(end)"===r.id&&(h="S0207");var g={code:h,position:r.position,token:r.value};if(t)return u.push(g),{type:"error",error:g};throw g.stack=(new Error).stack,g}return r.keepArray&&(n.keepArray=!0),n}(T)).type||void 0!==T.seekingParent)throw{code:"S0217",token:T.type,position:T.position};return u.length>0&&(T.errors=u),T});t.exports=s},{"./signature":5}],5:[function(e,t,r){"use strict";function n(e){"@babel/helpers - typeof";return(n="function"==typeof Symbol&&"symbol"==typeof Symbol.iterator?function(e){return typeof e}:function(e){return e&&"function"==typeof Symbol&&e.constructor===Symbol&&e!==Symbol.prototype?"symbol":typeof e})(e)}var a=e("./utils"),i=function(){var e={a:"arrays",b:"booleans",f:"functions",n:"numbers",o:"objects",s:"strings"};return function(t){for(var r=1,i=[],o={},s=o;r<t.length;){var u=t.charAt(r);if(":"===u)break;var c=function(){i.push(o),s=o,o={}},p=function(e,t,r,n){for(var a=1,i=t;i<e.length;)if(i++,(u=e.charAt(i))===n){if(0==--a)break}else u===r&&a++;return i};switch(u){case"s":case"n":case"b":case"l":case"o":o.regex="["+u+"m]",o.type=u,c();break;case"a":o.regex="[asnblfom]",o.type=u,o.array=!0,c();break;case"f":o.regex="f",o.type=u,c();break;case"j":o.regex="[asnblom]",o.type=u,c();break;case"x":o.regex="[asnblfom]",o.type=u,c();break;case"-":s.context=!0,s.contextRegex=new RegExp(s.regex),s.regex+="?";break;case"?":case"+":s.regex+=u;break;case"(":var f=p(t,r,"(",")"),l=t.substring(r+1,f);if(-1!==l.indexOf("<"))throw{code:"S0402",stack:(new Error).stack,value:l,offset:r};o.regex="["+l+"m]",o.type="("+l+")",r=f,c();break;case"<":if("a"!==s.type&&"f"!==s.type)throw{code:"S0401",stack:(new Error).stack,value:s.type,offset:r};var d=p(t,r,"<",">");s.subtype=t.substring(r+1,d),r=d}r++}var h="^"+i.map(function(e){return"("+e.regex+")"}).join("")+"$",g=new RegExp(h),v=function(e){var t;if(a.isFunction(e))t="f";else switch(n(e)){case"string":t="s";break;case"number":t="n";break;case"boolean":t="b";break;case"object":t=null===e?"l":Array.isArray(e)?"a":"o";break;case"undefined":default:t="m"}return t};return{definition:t,validate:function(t,r){var n="";t.forEach(function(e){n+=v(e)});var a=g.exec(n);if(a){var o=[],s=0;return i.forEach(function(n,i){var u=t[s],c=a[i+1];if(""===c)if(n.context&&n.contextRegex){var p=v(r);if(!n.contextRegex.test(p))throw{code:"T0411",stack:(new Error).stack,value:r,index:s+1};o.push(r)}else o.push(u),s++;else c.split("").forEach(function(r){if("a"===n.type){if("m"===r)u=void 0;else{u=t[s];var a=!0;if(void 0!==n.subtype)if("a"!==r&&c!==n.subtype)a=!1;else if("a"===r&&u.length>0){var i=v(u[0]);a=i===n.subtype.charAt(0)&&0===u.filter(function(e){return v(e)!==i}).length}if(!a)throw{code:"T0412",stack:(new Error).stack,value:u,index:s+1,type:e[n.subtype]};"a"!==r&&(u=[u])}o.push(u),s++}else o.push(u),s++})}),o}!function(e,t){for(var r="^",n=0,a=0;a<i.length;a++){r+=i[a].regex;var o=t.match(r);if(null===o)throw{code:"T0410",stack:(new Error).stack,value:e[n],index:n+1};n=o[0].length}throw{code:"T0410",stack:(new Error).stack,value:e[n],index:n+1}}(t,n)}}}}();t.exports=i},{"./utils":6}],6:[function(e,t,r){"use strict";function n(e){"@babel/helpers - typeof";return(n="function"==typeof Symbol&&"symbol"==typeof Symbol.iterator?function(e){return typeof e}:function(e){return e&&"function"==typeof Symbol&&e.constructor===Symbol&&e!==Symbol.prototype?"symbol":typeof e})(e)}var a=function(){function e(e){var t=!1;if("number"==typeof e&&(t=!isNaN(e))&&!isFinite(e))throw{code:"D1001",value:e,stack:(new Error).stack};return t}var t=("function"==typeof Symbol?Symbol:{}).iterator||"@@iterator";return{isNumeric:e,isArrayOfStrings:function(e){var t=!1;return Array.isArray(e)&&(t=0===e.filter(function(e){return"string"!=typeof e}).length),t},isArrayOfNumbers:function(t){var r=!1;return Array.isArray(t)&&(r=0===t.filter(function(t){return!e(t)}).length),r},createSequence:function(){var e=[];return e.sequence=!0,1===arguments.length&&e.push(arguments[0]),e},isSequence:function(e){return!0===e.sequence&&Array.isArray(e)},isFunction:function(e){return e&&(!0===e._jsonata_function||!0===e._jsonata_lambda)||"function"==typeof e},isLambda:function(e){return e&&!0===e._jsonata_lambda},isIterable:function(e){return"object"===n(e)&&null!==e&&t in e&&"next"in e&&"function"==typeof e.next},getFunctionArity:function(e){return"number"==typeof e.arity?e.arity:"function"==typeof e.implementation?e.implementation.length:"number"==typeof e.length?e.length:e.arguments.length},isDeepEqual:function e(t,r){if(t===r)return!0;if("object"===n(t)&&"object"===n(r)&&null!==t&&null!==r){if(Array.isArray(t)&&Array.isArray(r)){if(t.length!==r.length)return!1;for(var a=0;a<t.length;a++)if(!e(t[a],r[a]))return!1;return!0}var i=Object.getOwnPropertyNames(t),o=Object.getOwnPropertyNames(r);if(i.length!==o.length)return!1;for(i=i.sort(),o=o.sort(),a=0;a<i.length;a++)if(i[a]!==o[a])return!1;for(a=0;a<i.length;a++){var s=i[a];if(!e(t[s],r[s]))return!1}return!0}return!1},stringToArray:function(e){var t=[],r=!0,n=!1,a=void 0;try{for(var i,o=e[Symbol.iterator]();!(r=(i=o.next()).done);r=!0){var s=i.value;t.push(s)}}catch(e){n=!0,a=e}finally{try{r||null==o.return||o.return()}finally{if(n)throw a}}return t}}}();t.exports=a},{}]},{},[3])(3)});;// jsonata-es5.min.js is prepended to this file as part of the Grunt build

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
