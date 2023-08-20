
var Module = {
    print: (function(text) {
        console.log(text);
        var element = document.getElementById('try-output');
        if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
        element.innerText += text + "\n";
        element.scrollTop = element.scrollHeight;
    }),
        printErr: function(text) {
        console.warn(text);
        var element = document.getElementById('try-output');
        if(element.getAttribute('ready') === null) return;
        if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
        element.innerHTML += `<span class="token error">${{text}}</span>\n`;
        element.scrollTop = element.scrollHeight;
    }
};
