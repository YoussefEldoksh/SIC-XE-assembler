let submitBtn = document.querySelector(".submitBtn");
let clearBtn = document.querySelector(".clearBtn");
let textArea = document.querySelector(".input");
let outputArea = document.querySelector(".output");

submitBtn.addEventListener("click", async () => {
    let code = textArea.value;
    let lines = code.split("\n");
    let formattedCode = "";
    for (let line of lines) {
        let trimmedLine = line.trim();
        if (trimmedLine.length > 0) {
            formattedCode += trimmedLine + "\n";
        }
    }

    try {
        const response = await fetch('/submit', {
            method: 'POST',
            headers: {
                'Content-Type': 'text/plain',
            },
            body:formattedCode
        });

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        const result = await response.text();
        if (outputArea) {
            outputArea.value = result;
        }
    } catch (error) {
        console.error('Error:', error);
        if (outputArea) {
            outputArea.value = 'Error: ' + error.message;
        }
    }
});

clearBtn.addEventListener("click", () => {
    textArea.value = "";
    if (outputArea) {
        outputArea.value = "";
    }
});