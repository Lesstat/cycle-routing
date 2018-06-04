var map = L.map("mapid", { closePopupOnClick: false }).setView(
  [48.7456643, 9.1070856],
  13
);
var start = true;

var startPopup = L.popup({ autoClose: false });
var endPopup = L.popup({ autoClose: false });
var geoJson = L.layerGroup([]).addTo(map);
var towerLayer = L.layerGroup([]).addTo(map);

let canvas = document.getElementById("triangleSelector");
let canvasRgb = document.getElementById("triangleSelectorRGB");

L.tileLayer("http://{s}.tile.openstreetmap.org/{id}/{z}/{x}/{y}.png", {
  maxZoom: 18,
  attribution:
    'Map data &copy; <a href="http://openstreetmap.org">OpenStreetMap</a> contributors, ' +
    '<a href="http://creativecommons.org/licenses/by-sa/2.0/">CC-BY-SA</a>, ',
  id: ""
}).addTo(map);

document.addEventListener("keydown", event => {
  if (event.ctrlKey && event.altKey && event.key == "d") {
    //Code follows
    let debugLog = document.getElementById("debuglog");
    if (debugLog.hasAttribute("hidden")) {
      debugLog.removeAttribute("hidden");
    } else {
      debugLog.setAttribute("hidden", "");
    }
  }
});

function calcDistWithCurrentSelection() {
  let length = document.getElementById("length_percent").innerHTML;
  let height = document.getElementById("height_percent").innerHTML;
  let unsuitability = document.getElementById("road_percent").innerHTML;
  calcDist(length, height, unsuitability);
}

function onMapClick(e) {
  let id;
  id = "start";
  startPopup
    .setLatLng(e.latlng)
    .setContent("Start at " + e.latlng.toString())
    .addTo(map);
  getNode(id, e.latlng);
}

function onRightClick(e) {
  let id = "end";
  endPopup
    .setLatLng(e.latlng)
    .setContent("End at " + e.latlng.toString())
    .addTo(map);
  getNode(id, e.latlng);
}

map.on("click", onMapClick);
map.on("contextmenu", onRightClick);

function getNode(id, latlng) {
  let xmlhttp = new XMLHttpRequest();

  xmlhttp.onload = function() {
    if (xmlhttp.status == 200) {
      addToDebugLog("getNode", xmlhttp.response.debug);
      document.getElementById(id).innerHTML = xmlhttp.responseText;
      calcDistWithCurrentSelection();
    }
  };
  xmlhttp.open(
    "GET",
    "/node_at?lat=" + latlng.lat + "&lng=" + latlng.lng,
    true
  );
  xmlhttp.send();
}

function calcDist(length, height, unsuitability) {
  listOfRoutes = [];
  let xmlhttp = new XMLHttpRequest();

  xmlhttp.responseType = "json";
  xmlhttp.onload = function() {
    if (xmlhttp.status == 200) {
      addToDebugLog("single route", xmlhttp.response.debug);
      let myStyle = {
        color: "#3333FF",
        weight: 5,
        opacity: 0.65
      };
      document.getElementById("route_length").innerHTML =
        Math.round(xmlhttp.response.length / 1000) / 10;
      document.getElementById("route_height").innerHTML =
        xmlhttp.response.height / 10;
      document.getElementById("route_unsuitability").innerHTML =
        xmlhttp.response.unsuitability;
      geoJson.clearLayers();
      geoJson.addLayer(
        L.geoJSON(xmlhttp.response.route.geometry, { style: myStyle })
      );
    } else {
      document.getElementById("route_length").innerHTML = "Unknown";
      document.getElementById("route_height").innerHTML = "Unknown";
      document.getElementById("route_unsuitability").innerHTML = "Unknown";
    }
  };
  let s = document.getElementById("start").innerHTML;
  let t = document.getElementById("end").innerHTML;
  xmlhttp.open(
    "GET",
    "/route?s=" +
      s +
      "&t=" +
      t +
      "&length=" +
      length +
      "&height=" +
      height +
      "&unsuitability=" +
      unsuitability,
    true
  );
  xmlhttp.send();
}

function panOutMap() {
  let xmlhttp = new XMLHttpRequest();
  xmlhttp.responseType = "json";
  xmlhttp.onload = function() {
    if (xmlhttp.status == 200) {
      map.fitBounds(xmlhttp.response);
    }
  };

  xmlhttp.open("GET", "/map_coords");
  xmlhttp.send();
}

function alternativeRoutes(kind) {
  listOfRoutes = [];
  geoJson.clearLayers();
  let xmlhttp = new XMLHttpRequest();

  xmlhttp.responseType = "json";
  xmlhttp.onload = function() {
    if (xmlhttp.status == 200) {
      addToDebugLog("kind", xmlhttp.response.debug);

      let values = xmlhttp.response.config1.split("/");

      let myStyle1 = {
        color: gradientToColor(values),
        weight: 5,
        opacity: 0.65
      };
      drawTriangle(canvas);
      let coord = configToCoords(values.map(val => val / 100));

      drawDot(canvas, coord.x, coord.y, "blue");

      geoJson.addLayer(
        L.geoJSON(xmlhttp.response.route1.route.geometry, { style: myStyle1 })
      );

      let myStyle2 = {
        color: "#33FF00",
        weight: 5,
        opacity: 0.65
      };

      values = xmlhttp.response.config2.split("/");
      coord = configToCoords(values.map(val => val / 100));

      drawDot(canvas, coord.x, coord.y, "green");

      geoJson.addLayer(
        L.geoJSON(xmlhttp.response.route2.route.geometry, { style: myStyle2 })
      );
    } else {
      document.getElementById("route_length").innerHTML = "Unknown";
      document.getElementById("route_height").innerHTML = "Unknown";
      document.getElementById("route_unsuitability").innerHTML = "Unknown";
    }
  };
  let s = document.getElementById("start").innerHTML;
  let t = document.getElementById("end").innerHTML;
  xmlhttp.open("GET", "/alternative/" + kind + "?s=" + s + "&t=" + t);
  xmlhttp.send();
}
function rainbow(number) {
  let colors = [
    "#543005",
    "#8c510a",
    "#bf812d",
    "#dfc27d",
    "#f628c3",
    "#000000",
    "#c7ea25",
    "#80cdc1",
    "#35978f",
    "#01665e",
    "#003c30",
    "#a50026",
    "#d73027",
    "#f46d43",
    "#fdae61",
    "#fee08b",
    "#333333",
    "#d9ef8b",
    "#a6d96a",
    "#66bd63",
    "#1a9850",
    "#006837"
  ];
  return colors[number % colors.length];
}

function gradientToColor(config) {
  let rgb = config.map(val => Math.round(val * 255));
  var decColor = 0x1000000 + rgb[0] + 0x100 * rgb[1] + 0x10000 * rgb[2];
  return "#" + decColor.toString(16).substr(1);
}

var listOfRoutes = [];

function initializeCanvas() {
  canvas.addEventListener("mousemove", moveDot);
  canvas.addEventListener("mouseup", mouseUp);
  canvas.addEventListener("mousedown", mouseDown);

  canvasRgb.addEventListener("mousemove", moveDot);
  canvasRgb.addEventListener("mouseup", mouseUp);
  canvasRgb.addEventListener("mousedown", mouseDown);

  drawTriangle(canvas);
  drawTriangle(canvasRgb);
  drawDot(canvas, center.x, center.y);
  drawDot(canvasRgb, center.x, center.y);
}

const lengthCorner = { x: 5, y: 505 };
const heightCorner = { x: 505, y: 505 };
const unsuitabilityCorner = { x: 252, y: 22 };
const center = configToCoords([1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0]);

function drawTriangle(canvas) {
  let ctx = canvas.getContext("2d");
  ctx.clearRect(0, 0, canvas.width, canvas.height);

  ctx.fillStyle = "lightgrey";
  ctx.beginPath();
  ctx.moveTo(lengthCorner.x, lengthCorner.y);
  ctx.lineTo(heightCorner.x, heightCorner.y);
  ctx.lineTo(unsuitabilityCorner.x, unsuitabilityCorner.y);
  ctx.fill();
}

function drawDot(canvas, x, y, style) {
  if (style === undefined) {
    style = "black";
  }
  let ctx = canvas.getContext("2d");

  ctx.fillStyle = style;
  ctx.beginPath();
  ctx.arc(x, y, 3, 0, 2 * Math.PI);
  ctx.fill();
}

var clicked = false;

function mouseDown(event) {
  clicked = true;
  moveDot(event);
}

function mouseUp(event) {
  clicked = false;
  calcDistWithCurrentSelection();
}

function moveDot(event) {
  let lengthSpan = document.getElementById("length_percent");
  let heightSpan = document.getElementById("height_percent");
  let unsuitabilitySpan = document.getElementById("road_percent");

  if (clicked && event) {
    drawTriangle(canvas);
    drawDot(canvas, event.offsetX, event.offsetY);
    drawTriangle(canvasRgb);
    drawDot(canvasRgb, event.offsetX, event.offsetY);
    let point = { x: event.offsetX, y: event.offsetY };
    let lengthArea = triangleArea(point, heightCorner, unsuitabilityCorner);
    let heightArea = triangleArea(point, lengthCorner, unsuitabilityCorner);
    let unsuitabilityArea = triangleArea(point, lengthCorner, heightCorner);

    let areaSum = lengthArea + heightArea + unsuitabilityArea;

    let lengthPercent = Math.round(lengthArea / areaSum * 100);
    let heightPercent = Math.round(heightArea / areaSum * 100);

    let unsuitabilityPercent = Math.round(unsuitabilityArea / areaSum * 100);

    lengthSpan.innerHTML = lengthPercent;
    heightSpan.innerHTML = heightPercent;
    unsuitabilitySpan.innerHTML = unsuitabilityPercent;
  } else if (event && listOfRoutes.length) {
    let point = { x: event.offsetX, y: event.offsetY };
    let minDist = 1000;
    let minIndex;
    for (let index in listOfRoutes) {
      listOfRoutes[index].route.setStyle({ weight: 4 });
      let dis = dist(listOfRoutes[index].point, point);
      if (minDist > dis) {
        minDist = dis;
        minIndex = index;
      }
    }
    if (minDist <= 5) {
      let bestRoute = listOfRoutes[minIndex];
      bestRoute.route.setStyle({ weight: 10 });
      let conf = bestRoute.config;
      lengthSpan.innerHTML = conf[0] * 100;
      heightSpan.innerHTML = conf[1] * 100;
      unsuitabilitySpan.innerHTML = conf[2] * 100;

      let cost = bestRoute.cost;

      document.getElementById("route_length").innerHTML =
        Math.round(cost.length / 1000) / 10;
      document.getElementById("route_height").innerHTML = cost.height / 10;
      document.getElementById("route_unsuitability").innerHTML =
        cost.unsuitability;
    }
  }
}

function dist(p1, p2) {
  return Math.sqrt((p1.x - p2.x) ** 2 + (p1.y - p2.y) ** 2);
}

function triangleArea(p1, p2, p3) {
  let a = dist(p1, p2);
  let b = dist(p1, p3);
  let c = dist(p2, p3);

  let s = (a + b + c) / 2;

  return Math.sqrt(s * (s - a) * (s - b) * (s - c));
}

function configToCoords(values) {
  return {
    x:
      lengthCorner.x * values[0] +
      heightCorner.x * values[1] +
      unsuitabilityCorner.x * values[2],
    y:
      lengthCorner.y * values[0] +
      heightCorner.y * values[1] +
      unsuitabilityCorner.y * values[2]
  };
}
function scalingTriangulation() {
  geoJson.clearLayers();
  let xmlhttp = new XMLHttpRequest();

  xmlhttp.responseType = "json";
  xmlhttp.onload = function() {
    if (xmlhttp.status == 200) {
      addToDebugLog("triangulation", xmlhttp.response.debug);
      listOfRoutes = [];
      drawTriangle(canvas);
      drawTriangle(canvasRgb);

      let points = xmlhttp.response.points;
      let triangles = xmlhttp.response.triangles;

      drawTriangles(canvas.getContext("2d"), points, triangles, false);
      drawTriangles(canvasRgb.getContext("2d"), points, triangles, true);

      for (let p in points) {
        let values = points[p].conf.split("/");
        let col = gradientToColor(values);
        let coord = configToCoords(values);
        drawDot(canvas, coord.x, coord.y, rainbow(p));
        drawDot(canvasRgb, coord.x, coord.y, col);

        let myStyle = {
          color: rainbow(p),
          weight: 4,
          opacity: 1
        };
        let geoRoute = L.geoJSON(points[p].route.route.geometry, {
          style: myStyle
        });

        geoJson.addLayer(geoRoute);
        listOfRoutes.push({
          point: coord,
          route: geoRoute,
          config: values,
          cost: {
            length: points[p].route.length,
            height: points[p].route.height,
            unsuitability: points[p].route.unsuitability
          }
        });
      }
    }
  };

  let s = document.getElementById("start").innerHTML;
  let t = document.getElementById("end").innerHTML;
  let maxSplits = document.getElementById("maxSplits").value;
  let maxLevel = document.getElementById("maxLevel").value;
  let splitByLevel = document.getElementById("splitByLevel").checked;
  let uri = "/scaled" + "?s=" + s + "&t=" + t + "&maxSplits=" + maxSplits;

  if (maxLevel > 0) {
    uri += "&maxLevel=" + maxLevel;
  }
  if (splitByLevel) {
    uri += "&splitByLevel=" + "true";
  }

  xmlhttp.open("GET", uri);
  xmlhttp.send();
}

function addToDebugLog(requestType, message) {
  if (!message) {
    return;
  }
  let debugLog = document.getElementById("debuglog");
  let content = "Debug log for " + requestType + " request\n";
  content += message;
  content += "End of log for " + requestType + " request\n";
  content += "=============================================\n";
  debugLog.value += content;
}

function drawTriangles(ctx, points, triangles, gradient) {
  for (let t in triangles) {
    ctx.fillStyle = "black";
    let config1 = points[triangles[t].point1].conf
      .split("/")
      .map(val => parseFloat(val));
    let config2 = points[triangles[t].point2].conf
      .split("/")
      .map(val => parseFloat(val));
    let config3 = points[triangles[t].point3].conf
      .split("/")
      .map(val => parseFloat(val));
    let point1 = configToCoords(config1);
    let point2 = configToCoords(config2);
    let point3 = configToCoords(config3);

    ctx.beginPath();
    ctx.moveTo(point1.x, point1.y);
    ctx.lineTo(point2.x, point2.y);
    ctx.lineTo(point3.x, point3.y);
    let filled = false;
    if (gradient) {
      if (triangles[t].noChildren) {
        let center = [];
        for (let i = 0; i < 3; i++) {
          center.push((config1[i] + config2[i] + config3[i]) / 3);
        }
        ctx.fillStyle = gradientToColor(center);
        filled = true;
      }
    } else {
      if (triangles[t].noMoreRoutes) {
        ctx.fillStyle = rainbow(triangles[t].point1);
        filled = true;
      }
    }
    if (filled) {
      ctx.fill();
    } else {
      ctx.stroke();
    }
  }
}
