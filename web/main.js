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
            body: formattedCode
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

document.addEventListener('DOMContentLoaded', function () {
    const scrollContainer = document.getElementById("scrollList");
    const items = scrollContainer.querySelectorAll(".scroll-list__item");
    const lastIndex = items.length - 1;

    // Set initial scroll position to the first item
    scrollContainer.scrollTop = 0;

    function updateEffects() {
        const containerRect = scrollContainer.getBoundingClientRect();
        const containerCenter = containerRect.top + containerRect.height / 2;
        const scrollPosition = scrollContainer.scrollTop;
        const maxScroll = scrollContainer.scrollHeight - scrollContainer.clientHeight;
        const isNearTop = scrollPosition < 50;
        const isNearBottom = scrollPosition > maxScroll - 50;

        items.forEach((item, index) => {
            const itemRect = item.getBoundingClientRect();
            const itemCenter = itemRect.top + itemRect.height / 2;
            const distance = Math.abs(containerCenter - itemCenter);
            const ratio = Math.max(0, 1 - distance / (containerRect.height / 2));

            // Handle first item
            if (index === 0) {
                if (isNearTop) {
                    item.style.transform = 'scale(1.1)';
                    item.style.opacity = '0.9';
                    item.classList.add("active");
                } else {
                    item.style.transform = `scale(${0.5 + ratio * 0.7})`;
                    item.style.opacity = 0.2 + ratio * 0.8;
                    if (ratio > 0.95 && !isNearBottom) {
                        item.classList.add("active");
                    } else {
                        item.classList.remove("active");
                    }
                }
            }
            // Handle last item
            else if (index === lastIndex) {
                if (isNearBottom) {
                    item.style.transform = 'scale(1.1)';
                    item.style.opacity = '0.9';
                    item.classList.add("active");
                } else {
                    item.style.transform = `scale(${0.5 + ratio * 0.7})`;
                    item.style.opacity = 0.2 + ratio * 0.8;
                    if (ratio > 0.95 && !isNearTop) {
                        item.classList.add("active");
                    } else {
                        item.classList.remove("active");
                    }
                }
            }
            // Handle middle items
            else {
                // Reduce scaling for items after the first one when near top
                const adjustedRatio = (isNearTop && index < 2) ? ratio * 0.5 :
                    (isNearBottom && index > lastIndex - 2) ? ratio * 0.5 :
                        ratio;
                item.style.transform = `scale(${0.5 + adjustedRatio * 0.7})`;
                item.style.opacity = 0.2 + adjustedRatio * 0.8;
                if (ratio > 0.95 && !isNearTop && !isNearBottom) {
                    item.classList.add("active");
                } else {
                    item.classList.remove("active");
                }
            }
        });
    }

    // Add scroll event listener
    scrollContainer.addEventListener("scroll", updateEffects);
    // Initial check on load
    window.addEventListener("load", updateEffects);
    // Force initial update
    updateEffects();
});