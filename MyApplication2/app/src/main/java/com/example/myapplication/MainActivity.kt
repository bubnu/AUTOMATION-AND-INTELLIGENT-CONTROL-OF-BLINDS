package com.example.myapplication

import android.annotation.SuppressLint
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Info
import androidx.compose.material.icons.filled.NightsStay
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.example.myapplication.ui.theme.MyApplicationTheme
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.coroutineScope
import kotlinx.coroutines.currentCoroutineContext
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import okhttp3.Call
import okhttp3.Callback
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.Response
import org.json.JSONObject
import java.io.IOException

private val Color.Companion.DarkBlue: Color
    get() = Color(0xFF00008B)

class MainActivity : ComponentActivity() {
    private val client = OkHttpClient()
    private val BASE_URL = "http://192.168.1.140"

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            MyApplicationTheme {
                MainScreen(client, BASE_URL)
            }
        }
    }
}

@Composable
fun MainScreen(client: OkHttpClient, nodeMcuBaseUrl: String) {
    var activeMode by remember { mutableStateOf("none") } // none, manual, automatic, night
    var showInfoDialog by remember { mutableStateOf(false) }
    var showSettingsDialog by remember { mutableStateOf(false) }
    var isNightMode by remember { mutableStateOf(false) }
    var isDarkTheme by remember { mutableStateOf(false) }
    var lastMode by remember { mutableStateOf("none") } // none, manual, automatic

    // Example of switching modes
    fun send_new_mode(newMode: String) {
        if(newMode != activeMode) {
            val url = "$nodeMcuBaseUrl/mode?newMode=$newMode"
            println(url)
            val request = Request.Builder().url(url).build()
            client.newCall(request).enqueue(object : Callback {
                override fun onFailure(call: Call, e: IOException) { e.printStackTrace() }
                override fun onResponse(call: Call, response: Response) { println(response) }
            })
        }
    }

    fun send_slider_data(sliderName: String, newValue: Float) {
        val url = "$nodeMcuBaseUrl/slider?slider=$sliderName&value=${newValue.toInt()}"
        val request = Request.Builder().url(url).build()
        client.newCall(request).enqueue(object : Callback {
            override fun onFailure(call: Call, e: IOException) { e.printStackTrace() }
            override fun onResponse(call: Call, response: Response) { println(response) }
        })
    }

    MyApplicationTheme(darkTheme = isDarkTheme) {

        Box(modifier = Modifier.fillMaxSize()) {
            // Main content layout
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .background(MaterialTheme.colorScheme.background)
            ) {
                Box(
                    modifier = Modifier
                        .weight(0.85f)
                        .fillMaxWidth()
                ) {
                    Column {
                        ManualMode(
                            modifier = Modifier.weight(1f).fillMaxWidth().clickable {
                                send_new_mode("manual")
                                activeMode = "manual"
                                isNightMode = false
                            },
                            isActive = activeMode == "manual" && !isNightMode,
                            onSliderChange = {sliderName, newValue ->
                                send_new_mode("manual")
                                activeMode = "manual"
                                isNightMode = false
                                send_slider_data(sliderName, newValue)
                            }
                        )
                        AutomaticMode(
                            modifier = Modifier.weight(1f).fillMaxWidth().clickable {
                                send_new_mode("automatic")
                                activeMode = "automatic"
                                isNightMode = false
                            },
                            isActive = activeMode == "automatic" && !isNightMode,
                            onSliderChange = {sliderName, newValue ->
                                send_new_mode("automatic")
                                activeMode = "automatic"
                                isNightMode = false
                                send_slider_data(sliderName, newValue)
                            },
                            client,
                            nodeMcuBaseUrl
                        )
                    }
                }
                BottomToolbar(
                    onInfoClicked = { showInfoDialog = true },
                    onNightModeToggled = {
                        isNightMode = !isNightMode
                        if(isNightMode){
                            send_new_mode("night")
                            lastMode = activeMode
                            activeMode = "night"
                        }else{
                            send_new_mode(lastMode)
                            activeMode = lastMode
                        }
                                         },
                    onSettingsClicked = { showSettingsDialog = true },
                    isNightMode = isNightMode,
                    modifier = Modifier.weight(0.15f)
                )
            }

            // Dialogs
            if (showInfoDialog) {
                InfoDialog(onDismissRequest = { showInfoDialog = false })
            }

            if (showSettingsDialog) {
                SettingsDialog(isDarkTheme = isDarkTheme, onToggleDarkTheme = { isDarkTheme = !isDarkTheme }, onDismissRequest = { showSettingsDialog = false })
            }
        }
    }

}


@Composable
fun ManualMode(modifier: Modifier = Modifier, isActive: Boolean, onSliderChange: (sliderName: String, newValue: Float) -> Unit) {
    var lightIntensity by remember { mutableStateOf(0f) }
    var curtainPosition by remember { mutableStateOf(0f) }

    Column(
        modifier = modifier
            .padding(16.dp)
            .border(
                width = if (isActive) 4.dp else 0.dp,
                color = if (isActive) MaterialTheme.colorScheme.secondary else MaterialTheme.colorScheme.onSurface.copy(alpha = 0.12f),
                shape = RoundedCornerShape(8.dp)
            )
            .padding(10.dp),
        verticalArrangement = Arrangement.Top,
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Text(
            text = "Manual Mode",
            style = MaterialTheme.typography.titleLarge,
            color = MaterialTheme.colorScheme.onBackground
        )
        Spacer(modifier = Modifier.height(42.dp))
        Text(
            text = if (isActive) "Light Intensity: ${Math.round(lightIntensity)}%" else "Light Intensity",
            style = MaterialTheme.typography.bodyLarge,
            color = MaterialTheme.colorScheme.onBackground
        )
        Slider(
            value = lightIntensity,
            onValueChange = {
                lightIntensity = it
                onSliderChange("lightIntensity", it)
            },
            valueRange = 0f..100f,
            colors = SliderDefaults.colors(
                thumbColor = MaterialTheme.colorScheme.primary,
                activeTrackColor = MaterialTheme.colorScheme.primary.copy(alpha = 0.5f)
            ),
            modifier = Modifier.fillMaxWidth()
        )
        Spacer(modifier = Modifier.height(16.dp))
        Text(
            text = if (isActive) "Curtain: ${Math.round(curtainPosition)}%" else "Curtain",
            style = MaterialTheme.typography.bodyLarge,
            color = MaterialTheme.colorScheme.onBackground
        )
        Slider(
            value = curtainPosition,
            onValueChange = {
                curtainPosition = it
                onSliderChange("curtain", it)
            },
            valueRange = 0f..100f,
            colors = SliderDefaults.colors(
                thumbColor = MaterialTheme.colorScheme.primary,
                activeTrackColor = MaterialTheme.colorScheme.primary.copy(alpha = 0.5f)
            ),
            modifier = Modifier.fillMaxWidth()
        )
    }
}

@Composable
fun AutomaticMode(modifier: Modifier = Modifier, isActive: Boolean, onSliderChange: (sliderName: String, newValue: Float) -> Unit, client: OkHttpClient, nodeMcuBaseUrl: String) {
    var sliderValue by remember { mutableStateOf(0f) }
    var lightIntensity by remember { mutableStateOf(70) }  // Dummy value for light intensity
    var curtainPosition by remember { mutableStateOf(30) }  // Dummy value for curtain position

    // This LaunchedEffect will restart whenever `isActive` changes.
    LaunchedEffect(isActive) {
        if (isActive) {
            while (isActive && currentCoroutineContext().isActive) { // Check if still active
                try {
                    // Perform network operation on IO dispatcher
                    withContext(Dispatchers.IO) {
                        val request = Request.Builder().url("$nodeMcuBaseUrl/sensors").build()
                        val response = client.newCall(request).execute()
                        val jsonData = response.body?.string()
                        jsonData?.let {
                            val jsonObject = JSONObject(it)
                            withContext(Dispatchers.Main) { // Switch back to main thread to update state
                                lightIntensity = jsonObject.getInt("lightIntensity")
                                curtainPosition = jsonObject.getInt("curtainPosition")
                            }
                        }
                    }
                } catch (e: Exception) {
                    e.printStackTrace()
                }
//                delay(1000) // Fetch every second
            }
        }
    }

    Column(
        modifier = modifier
            .padding(16.dp)
            .border(
                width = if (isActive) 4.dp else 0.dp,
                color = if (isActive) MaterialTheme.colorScheme.secondary else MaterialTheme.colorScheme.onSurface.copy(alpha = 0.12f),
                shape = RoundedCornerShape(8.dp)
            )
            .padding(10.dp),
        verticalArrangement = Arrangement.Top,
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Text(
            text = "Automatic Mode",
            style = MaterialTheme.typography.titleLarge,
            color = MaterialTheme.colorScheme.onBackground
        )
        Spacer(modifier = Modifier.height(42.dp))
        Text(
            text = if (isActive) "Room Intensity: ${Math.round(sliderValue)}%" else "Room Intensity",
            style = MaterialTheme.typography.bodyLarge,
            color = MaterialTheme.colorScheme.onBackground
        )
        Slider(
            value = sliderValue,
            onValueChange = {
                sliderValue = it
                onSliderChange("roomIntensity", it)
            },
            valueRange = 0f..100f,
            colors = SliderDefaults.colors(
                thumbColor = MaterialTheme.colorScheme.primary,
                activeTrackColor = MaterialTheme.colorScheme.primary.copy(alpha = 0.5f)
            ),
            modifier = Modifier.fillMaxWidth()
        )
        Spacer(modifier = Modifier.height(30.dp))
        // Additional information displayed only when active
        if (isActive) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text(
                    text = "Light Intensity: ${lightIntensity}%",
                    style = MaterialTheme.typography.bodyLarge,
                    color = MaterialTheme.colorScheme.onBackground
                )
                Text(
                    text = "Curtain: ${curtainPosition}%",
                    style = MaterialTheme.typography.bodyLarge,
                    color = MaterialTheme.colorScheme.onBackground
                )
            }
        }
    }
}


@Composable
fun BottomToolbar(
    onInfoClicked: () -> Unit,
    onNightModeToggled: () -> Unit,
    onSettingsClicked: () -> Unit,
    isNightMode: Boolean,
    modifier: Modifier = Modifier
) {
    Row(
        modifier = modifier
            .fillMaxWidth()
            .padding(16.dp),
        horizontalArrangement = Arrangement.SpaceEvenly,
        verticalAlignment = Alignment.CenterVertically
    ) {
        IconButton(onClick = onInfoClicked, imageVector = Icons.Default.Info, contentDescription = "Info", isNightMode = false)
        IconButton(onClick = onNightModeToggled, imageVector = Icons.Default.NightsStay, contentDescription = "Night Mode", isNightMode = isNightMode)
        IconButton(onClick = onSettingsClicked, imageVector = Icons.Default.Settings, contentDescription = "Settings", isNightMode = false)
    }
}

@Composable
fun IconButton(onClick: () -> Unit, imageVector: ImageVector, contentDescription: String, isNightMode: Boolean) {
    IconButton(onClick = onClick) {
        Icon(
            imageVector = imageVector,
            contentDescription = contentDescription,
            tint = if (isNightMode) MaterialTheme.colorScheme.primary else MaterialTheme.colorScheme.onSurface,
            modifier = Modifier.size(48.dp).background(
                color = if (isNightMode) MaterialTheme.colorScheme.onPrimary else MaterialTheme.colorScheme.surface,
                shape = CircleShape
            )
        )
    }
}


@SuppressLint("RememberReturnType")
@Composable
fun InfoDialog(onDismissRequest: () -> Unit) {
    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(Color.Black.copy(alpha = 0.5f))  // Background around the dialog is 50% transparent
            .clickable(onClick = onDismissRequest, indication = null, interactionSource = remember { MutableInteractionSource() }),
        contentAlignment = Alignment.Center
    ) {
        Surface(
            modifier = Modifier.padding(32.dp),
            shape = RoundedCornerShape(8.dp),
            color = MaterialTheme.colorScheme.surface // Text box is fully opaque
        ) {
            Column(modifier = Modifier.padding(16.dp)) {
                Text(
                        "Manual Mode\n" +
                            "You can manually control the position of the blind and the LED intensity using the sliders in the application, allowing for precise adjustments according to your personal preferences.\n" +
                            "\n" +
                            "Automatic Mode\n" +
                            "The system automatically adjusts the blind and LED to maintain the desired light level, set by you through the slider in the application.\n" +
                            "\n" +
                            "Night Mode\n" +
                            "The blind and LED are completely closed to ensure a dark environment, optimal for sleep.",
                    style = MaterialTheme.typography.bodyLarge
                )
            }
        }
    }
}

@SuppressLint("RememberReturnType")
@Composable
fun SettingsDialog(isDarkTheme: Boolean, onToggleDarkTheme: () -> Unit, onDismissRequest: () -> Unit) {
    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(Color.Black.copy(alpha = 0.5f))
            .clickable(onClick = onDismissRequest, indication = null, interactionSource = remember { MutableInteractionSource() }),
        contentAlignment = Alignment.Center
    ) {
        Surface(
            modifier = Modifier.padding(32.dp),
            shape = RoundedCornerShape(8.dp),
            color = MaterialTheme.colorScheme.surface
        ) {
            Column(
                horizontalAlignment = Alignment.CenterHorizontally,
                modifier = Modifier.padding(16.dp)
            ) {
                Text("Settings", style = MaterialTheme.typography.headlineMedium)
                Spacer(modifier = Modifier.height(20.dp))
                Text("Toggle dark mode", style = MaterialTheme.typography.bodyLarge)
                Switch(checked = isDarkTheme, onCheckedChange = { onToggleDarkTheme() })
            }
        }
    }
}

@Preview(showBackground = true)
@Composable
fun DefaultPreview() {
    val client = OkHttpClient()
    val BASE_URL = "http://192.168.88.217"
    MyApplicationTheme {
        MainScreen(client, BASE_URL)
    }
}
