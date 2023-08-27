import {CodeJar} from '/codejar.js'

window.onload = function() {
    var blocks = document.querySelectorAll('pre.snippet')
    blocks.forEach((element) => {
        var lang = 'mosc'
        var input_lang = element.getAttribute('data-lang')
        if(input_lang) lang = input_lang
        var code = document.createElement('code');
    code.setAttribute('class', ' language-'+lang);
    code.innerHTML = element.innerHTML;
    element.innerHTML = '';
    element.append(code)
});
    Prism.highlightAll();

    var try_code = document.querySelector("#try-code")
    if(try_code) {
        var jar_options = { tab: ' '.repeat(2) }
        var jar = CodeJar(try_code, withLineNumbers(Prism.highlightElement), jar_options)
        var output = document.querySelector("#try-output")
        var result = document.querySelector("#try-result")
        Module.print = function(text) { output.innerText += text + "\n"; }
        Module.printErr = function(text) { output.innerText += text + "\n"; }

        var run = document.querySelector("#try-run")
        var share = document.querySelector("#share")
        var hello = document.querySelector("#try-hello")
        var fractal = document.querySelector("#try-fractal")
        var loop = document.querySelector("#try-loop")
        var copiedPopup = document.querySelector("#copied-popup p")
        var compile = Module.cwrap('mosc_compile', 'number', ['string'])

        var set_input = (content) => {
            output.innerText = '...';
            result.removeAttribute('class');
            result.innerText = 'no errors';
            jar.updateCode(content);
        }

        share.onclick = (e) => {
            var code = jar.toString()
            var compressed = LZString.compressToEncodedURIComponent(code)
            var url = location.protocol + "//" + location.host + location.pathname + "?code=" + compressed
            navigator.clipboard.writeText(url).then(
                () => {
                copiedPopup.style.opacity = "1"
            setTimeout(() => {
                copiedPopup.style.opacity = ""
        }, 1000)
        },
            (e) => console.error(e)
        )
        }

        run.onclick = (e) => {
            console.log("run")
            output.setAttribute('ready', '');
            output.innerText = '';
            var res = compile(jar.toString())
            var message = "no errors!"
            result.removeAttribute('class');
            if(res === 0) { //MSCRESULT_COMPILE_ERROR
                message = "Compile error!"
                result.setAttribute('class', 'error');
            } else if(res === 1) { //WREN_RESULT_RUNTIME_ERROR
                message = "Runtime error!"
                result.setAttribute('class', 'error');
            }
            result.innerText = message;
            console.log(result);
        }

        hello.onclick = (e) => { set_input('A.yira("hello mosc")') }
        loop.onclick = (e) => { set_input(`seginka 1..10 kono i A.yira("Counting up $i")`); }
        fractal.onclick = (e) => {
            set_input(`seginka (0...24 kono yPixel) {
  nin y = yPixel / 12 - 1
  seginka (0...80 kono xPixel) {
    nin x = xPixel / 30 - 2
    nin x0 = x
    nin y0 = y
    nin iter = 0
    foo (iter < 11 && x0 * x0 + y0 * y0 <= 4) {
      nin x1 = (x0 * x0) - (y0 * y0) + x
      nin y1 = 2 * x0 * y0 + y
      x0 = x1
      y0 = y1
      iter = iter + 1
    }
    A.seben(" .-:;+=xX$& "[iter])
  }
  A.yira("")
}`);
        } //fractal

        var initial_code = new URLSearchParams(location.search).get("code")
        if (initial_code !== null) {
            initial_code = LZString.decompressFromEncodedURIComponent(initial_code)
            set_input(initial_code)
        }

    } //if try_code
}