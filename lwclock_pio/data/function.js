async function load() {

  populate_select_options("clockFrom", true);
  populate_select_options("clockTo", false);
  populate_select_options("dateFrom", true);
  populate_select_options("dateTo", false);
  populate_select_options("txtFrom0", true);
  populate_select_options("txtTo0", false);
  populate_select_options("txtFrom1", true);
  populate_select_options("txtTo1", false);
  populate_select_options("txtFrom2", true);
  populate_select_options("txtTo2", false);
  populate_select_options("txtFrom3", true);
  populate_select_options("txtTo3", false);
  populate_select_options("global_start", true);
  populate_select_options("global_stop", false);

  const resp = await fetch("/configs.json");
  const body = await resp.text();

  console.log(body);

  var data2 = JSON.parse(body);
  data = document.getElementsByTagName('body')[0].innerHTML;
  var new_string;
  for (var key in data2) {
    new_string = data.replace(new RegExp('{{' + key + '}}', 'g'), data2[key]);
    data = new_string;
  }
  document.getElementsByTagName('body')[0].innerHTML = new_string;
  var inputs = document.getElementsByTagName("input");
  var selects = document.getElementsByTagName("select");
  console.log(selects);

  for (var name in langRU) {
    if (document.getElementById(name)) {
      document.getElementById(name).innerHTML = langRU[name];
    }
  }

  for (var key in data2) {
    if (data2[key] == 'checked') {
      for (var i = 0; i < inputs.length; i++) {
        if (inputs[i].id === key) {
          inputs[i].checked = "true";
        }
      }
    }
    for (var i = 0; i < selects.length; i++) {
      if (selects[i].id === key) {
        document.getElementById(key).value = data2[key];
      }
    }
  }
}
function val(id) {
  var v = document.getElementById(id).value;
  return v;
}
function val_sw(nameSwitch) {
  switchOn = document.getElementById(nameSwitch);
  if (switchOn.checked) {
    return 1;
  }
  return 0;
}

async function restartEsp() {
  if (confirm("Restart ESP ?")) {
    await fetch("/restart?device=ok",
      {
        method: "POST"
      });
    window.location.reload();
  }
}

async function clearConfigEsp() {
  if (confirm("Clear ESP config ?")) {
    await fetch("/resetConfig?device=ok",
      {
        method: "POST"
      });
    window.location.reload();
  }
}


function populate_select_options(id, is_from) {
  var ops = [];

  if (is_from) {
    for (let i = 0; i < 24; i++) {
      s1 = (i < 10) ? ("0" + i) : i;
      s2 = s1 + ":30";
      s1 = s1 + ":00";
      v1 = i;
      v2 = i + 0.3;
      ops.push({ "value": v1, "text": s1 });
      ops.push({ "value": v2, "text": s2 });
    }
  } else {
    for (let i = 0; i < 24; i++) {
      s1 = (i < 10) ? ("0" + i) : i;
      s2 = ((i + 1) < 10) ? ("0" + (i + 1)) : (i + 1);
      s1 = s1 + ":30";
      s2 = s2 + ":00";
      v1 = i + 0.3;
      v2 = i + 1;
      ops.push({ "value": v1, "text": s1 });
      ops.push({ "value": v2, "text": s2 });
    }
  }

  var select = document.getElementById(id);
  for (let i = 0; i < ops.length; i++) {
    op = document.createElement("option");
    op.setAttribute("value", ops[i].value);
    op.appendChild(document.createTextNode(ops[i].text));
    select.appendChild(op);
  }
}
