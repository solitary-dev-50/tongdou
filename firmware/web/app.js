const personality = document.querySelector("#personality");

personality?.addEventListener("change", (event) => {
  console.log("personality changed", event.target.value);
});
