const personality = document.querySelector("#personality");
const apiList = document.querySelector("#api-list");

const scenarioApiRoutes = [
  "GET /api/health",
  "GET /api/scenario/config",
  "PUT /api/scenario/config",
  "POST /api/scenario/reset",
  "GET /api/scenario/export",
  "POST /api/scenario/import",
  "GET /api/scenario/options",
];

personality?.addEventListener("change", (event) => {
  console.log("personality changed", event.target.value);
});

if (apiList) {
  apiList.innerHTML = scenarioApiRoutes.map((route) => `<li>${route}</li>`).join("");
}
