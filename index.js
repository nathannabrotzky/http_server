document.addEventListener("DOMContentLoaded", function() {
    const button = document.getElementById("clickMe");
    const message = document.getElementById("message");

    button.addEventListener("click", function() {
        message.textContent = "Your server is working! 🎉";
    });
});