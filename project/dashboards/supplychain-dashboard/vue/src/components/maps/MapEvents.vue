<template>
    <div ref="mapContainer" class="w-full h-96 rounded-lg overflow-hidden"></div>
</template>


<script setup lang="ts">
import { ref, onMounted, watch, onBeforeUnmount } from 'vue';
import L, { Map as LeafletMap, Marker, Polyline } from 'leaflet';

interface LocationEvent {
    lat: number;
    lng: number;
    title: string;
    description?: string;
    type?: string;
}

const props = defineProps<{
    eventGroups: LocationEvent[][];
    eventsNoPath?: LocationEvent[];
    iconMap?: Record<string, L.Icon | L.DivIcon>;
    defaultIcon?: L.Icon | L.DivIcon;
    tileUrl?: string;
    tileOptions?: Record<string, any>;
    center?: [number, number];
    zoom?: number;
    pathColors?: string[];
}>();

const mapContainer = ref<HTMLDivElement>();
let map: LeafletMap;
let markers: Marker[] = [];
let pathLayers: Polyline[] = [];

// fallback palette
const defaultColors = ['blue', 'red', 'green', 'orange', 'purple'];

onMounted(() => {
    map = L.map(mapContainer.value!, {
        center: props.center ?? [0, 0],
        zoom: props.zoom ?? 2,
    });

    L.tileLayer(
        props.tileUrl ?? 'https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png',
        props.tileOptions ?? {
            attribution:
                'Map data © <a href="https://openstreetmap.org">OpenStreetMap</a> contributors',
        }
    ).addTo(map);

    updateAll();
});

watch(
    () => [props.eventGroups, props.eventsNoPath, props.pathColors],
    () => updateAll(),
    { deep: true }
);

onBeforeUnmount(() => {
    map.remove();
});

function updateAll() {
    clearAll();
    drawGroups();
    drawNoPath();
    fitToBounds();
}

function clearAll() {
    markers.forEach((m) => m.remove());
    pathLayers.forEach((p) => p.remove());
    markers = [];
    pathLayers = [];
}

function drawGroups() {
    props.eventGroups.forEach((group, idx) => {
        // pick from user-supplied pathColors, or fallback to defaults
        const color =
            (props.pathColors && props.pathColors[idx]) ??
            defaultColors[idx % defaultColors.length];
        const latlngs = group.map((e) => [e.lat, e.lng] as [number, number]);
        const poly = L.polyline(latlngs, { color, weight: 4 }).addTo(map);

        pathLayers.push(poly);

        group.forEach((e) => {
            const icon =
                props.iconMap?.[e.type ?? ''] ?? props.defaultIcon ?? undefined;
            const opts: any = {};

            if (icon) opts.icon = icon;

            const m = L.marker([e.lat, e.lng], opts)
                .addTo(map)
                .bindPopup(
                    `<strong>${e.title}</strong>` +
                    (e.description ? `<br/>${e.description}` : '')
                );

            markers.push(m);
        });
    });
}

function drawNoPath() {
    (props.eventsNoPath ?? []).forEach((e) => {
        const icon =
            props.iconMap?.[e.type ?? ''] ?? props.defaultIcon ?? undefined;
        const opts: any = {};
        if (icon) opts.icon = icon;
        const m = L.marker([e.lat, e.lng], opts)
            .addTo(map)
            .bindPopup(
                `<strong>${e.title}</strong>` +
                (e.description ? `<br/>${e.description}` : '')
            );
        markers.push(m);
    });
}

function fitToBounds() {
    const allLayers = [...markers, ...pathLayers];
    if (allLayers.length) {
        const group = L.featureGroup(allLayers as any);
        map.fitBounds(group.getBounds().pad(0.2));
    }
}
</script>
<style scoped>
/* ensure container has height */
.map-container {
    height: 100%;
}
</style>
