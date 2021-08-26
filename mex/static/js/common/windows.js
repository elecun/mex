
//device manager window
var _g_show_system_settings = false;
function windowSystemSettings(url){
    if(_g_show_system_settings)
        return;

    var submenu = [
        {   //reload page
            html: "<i class='fas fa-sync-alt'></i>",
            cls: "sys-button",
            onclick: "document.getElementById('window_system_settings_view').contentWindow.location.reload()"
        }
    ];
    
    Desktop.createWindow({
        customButtons: submenu,
        resizeable: true,
        draggable: true,
        width: 1000,
        height:500,
        icon: "<span class='fas fa-cog'></span>",
        title: "System Settings",
        shadow:true,
        place:"auto",
        clsContent: "bg-white",
        content:"<iframe id='window_system_settings_view' src='"+url+"' style='display:inline-block;' height='450' width='100%'></iframe>",
        onShow: function(win){
            win = $(win);
            _g_show_system_settings = true;
            document.getElementById("window_system_settings_view").height = win[0].offsetHeight-48;
        },
        onClose: function(win){
            _g_show_system_settings = false;
            win = $(win);
        },
        onResizeStop: function(win){
            win = $(win);
            document.getElementById("window_system_settings_view").height = win[0].height-45;
        },
        onResize: function(win){
            win = $(win);
        }
    });    
}
