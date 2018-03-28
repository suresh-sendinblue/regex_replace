const Regex = require('./build/Release/regex_replace.node');
const Buffer = require('buffer').Buffer
const temp_str = require("fs").readFileSync("./data.txt", 'utf8');
//const temp_str = '<a href="https://www.w3schools.com">Visit W3Schools.com!</a>';
//const pattern = /(<(a|area)\s*\n*[^>]*)[ |\n]href[\s]*=[\s]*('|")(?!mailto:)(?!tel:)(?!#)((?:(?!\mailin-url\b)[^'|"])*)('|")/gi //js
const pattern = "(<(a|area)\s*\n*[^>]*)[ |\n]href[\s]*=[\s]*('|\")(?!mailto:)(?!tel:)(?!#)((?:(?!\mailin-url\b)[^'|\"])*)('|\")" //c++
const replace = '$1 href="$4"'
const buffStr = new Buffer(temp_str, "utf8");
//const buffPatt = new Buffer(pattern);
//const buffRep = new Buffer(replace);
console.time("time taken")
const result = Regex.RegexReplace(buffStr, pattern, replace)
//const result = Regex.RegexReplace(temp_str, pattern, replace)
//console.log(result)
console.timeEnd("time taken")