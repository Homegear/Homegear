define("ace/snippets/jsonata",["require","exports","module"],function(a,b,c){"use strict";var d="";for(var e in jsonata.functions)jsonata.functions.hasOwnProperty(e)&&(d+="# "+e+"\nsnippet "+e+"\n\t"+jsonata.getFunctionSnippet(e)+"\n");b.snippetText=d,b.scope="jsonata"});