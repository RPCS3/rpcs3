package net.rpcs3.ui.library

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.WindowInsetsSides
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.only
import androidx.compose.foundation.layout.systemBars
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Menu
import androidx.compose.material.icons.outlined.Close
import androidx.compose.material.icons.outlined.FilterList
import androidx.compose.material.icons.outlined.Search
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.shadow
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import net.rpcs3.ui.theme.SettingsStyle

enum class LibraryTab(val label: String) {
    Library("Library"),
    Compatibility("Compatibility")
}

enum class GameSort(val label: String) {
    NameAscending("Name A-Z"),
    NameDescending("Name Z-A")
}

@Composable
fun LibraryTopBar(
    selectedTab: LibraryTab,
    onTabSelected: (LibraryTab) -> Unit,
    searchQuery: String,
    onSearchQueryChange: (String) -> Unit,
    sort: GameSort,
    onSortChange: (GameSort) -> Unit,
    onMenuClick: () -> Unit,
    trailing: @Composable () -> Unit = {}
) {
    var searchOpen by remember { mutableStateOf(false) }
    var filterOpen by remember { mutableStateOf(false) }
    val focusRequester = remember { FocusRequester() }

    LaunchedEffect(searchOpen) {
        if (searchOpen) {
            runCatching { focusRequester.requestFocus() }
        } else {
            onSearchQueryChange("")
        }
    }

    Box(
        modifier = Modifier
            .windowInsetsPadding(
                WindowInsets.systemBars.only(
                    WindowInsetsSides.Horizontal + WindowInsetsSides.Top
                )
            )
            .fillMaxWidth()
            .height(64.dp)
    ) {
        if (!searchOpen) {
            Box(modifier = Modifier.align(Alignment.Center)) {
                TabPill(selectedTab = selectedTab, onTabSelected = onTabSelected)
            }
        } else {
            Box(
                modifier = Modifier
                    .align(Alignment.Center)
                    .fillMaxWidth()
                    .padding(start = 108.dp, end = 62.dp)
                    .height(40.dp)
                    .clip(RoundedCornerShape(18.dp))
                    .background(SettingsStyle.InputSurface)
                    .border(1.dp, SettingsStyle.InputBorder, RoundedCornerShape(18.dp))
                    .padding(horizontal = 14.dp),
                contentAlignment = Alignment.CenterStart
            ) {
                if (searchQuery.isEmpty()) {
                    Text(
                        text = "Search games",
                        color = SettingsStyle.TextDim,
                        fontSize = 13.sp
                    )
                }
                BasicTextField(
                    value = searchQuery,
                    onValueChange = onSearchQueryChange,
                    singleLine = true,
                    cursorBrush = SolidColor(SettingsStyle.AccentBlue),
                    textStyle = MaterialTheme.typography.bodyMedium.copy(
                        color = SettingsStyle.TextPrimary,
                        fontSize = 13.sp
                    ),
                    modifier = Modifier
                        .fillMaxWidth()
                        .focusRequester(focusRequester)
                )
            }
        }

        Row(
            modifier = Modifier
                .align(Alignment.CenterStart)
                .padding(start = 10.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            BarIcon(
                icon = Icons.Filled.Menu,
                contentDescription = "Menu",
                onClick = onMenuClick
            )
            Spacer(Modifier.width(4.dp))
            BarIcon(
                icon = if (searchOpen) Icons.Outlined.Close else Icons.Outlined.Search,
                contentDescription = "Search",
                onClick = { searchOpen = !searchOpen }
            )
        }

        Row(
            modifier = Modifier
                .align(Alignment.CenterEnd)
                .padding(end = 10.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            trailing()
            Box {
                BarIcon(
                    icon = Icons.Outlined.FilterList,
                    contentDescription = "Filter",
                    onClick = { filterOpen = true }
                )
                DropdownMenu(
                    expanded = filterOpen,
                    onDismissRequest = { filterOpen = false },
                    shape = RoundedCornerShape(10.dp),
                    containerColor = SettingsStyle.CardSurface
                ) {
                    GameSort.entries.forEach { option ->
                        DropdownMenuItem(
                            text = {
                                Text(
                                    text = option.label,
                                    color = if (option == sort) {
                                        SettingsStyle.AccentBlue
                                    } else {
                                        SettingsStyle.TextPrimary
                                    },
                                    fontSize = 13.sp
                                )
                            },
                            onClick = {
                                filterOpen = false
                                onSortChange(option)
                            }
                        )
                    }
                }
            }
        }
    }
}

@Composable
private fun TabPill(selectedTab: LibraryTab, onTabSelected: (LibraryTab) -> Unit) {
    Row(
        modifier = Modifier
            .height(44.dp)
            .shadow(8.dp, RoundedCornerShape(18.dp))
            .clip(RoundedCornerShape(18.dp))
            .background(SettingsStyle.CardSurface)
            .border(1.dp, SettingsStyle.CardBorder, RoundedCornerShape(18.dp))
            .padding(4.dp),
        horizontalArrangement = Arrangement.spacedBy(4.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        LibraryTab.entries.forEach { tab ->
            TabCell(
                label = tab.label,
                selected = tab == selectedTab,
                onClick = { onTabSelected(tab) }
            )
        }
    }
}

@Composable
private fun TabCell(label: String, selected: Boolean, onClick: () -> Unit) {
    val interaction = remember { MutableInteractionSource() }
    val scale by animateFloatAsState(if (selected) 1f else 0.98f, label = "tabScale")

    Box(
        modifier = Modifier
            .width(118.dp)
            .height(36.dp)
            .graphicsLayer {
                scaleX = scale
                scaleY = scale
            }
            .clip(RoundedCornerShape(14.dp))
            .background(
                if (selected) SettingsStyle.AccentBlue.copy(alpha = 0.16f) else Color.Transparent
            )
            .clickable(interactionSource = interaction, indication = null, onClick = onClick),
        contentAlignment = Alignment.Center
    ) {
        Text(
            text = label,
            color = if (selected) SettingsStyle.AccentBlue else SettingsStyle.TextSecondary,
            fontSize = 13.sp,
            fontWeight = if (selected) FontWeight.SemiBold else FontWeight.Normal
        )
    }
}

@Composable
private fun BarIcon(
    icon: androidx.compose.ui.graphics.vector.ImageVector,
    contentDescription: String,
    onClick: () -> Unit
) {
    Box(
        modifier = Modifier
            .size(42.dp)
            .clip(RoundedCornerShape(12.dp))
            .clickable(onClick = onClick),
        contentAlignment = Alignment.Center
    ) {
        Icon(
            imageVector = icon,
            contentDescription = contentDescription,
            tint = SettingsStyle.TextPrimary,
            modifier = Modifier.size(22.dp)
        )
    }
}

@Composable
fun LibrarySearchVisibility(visible: Boolean, content: @Composable () -> Unit) {
    AnimatedVisibility(visible = visible) { content() }
}
