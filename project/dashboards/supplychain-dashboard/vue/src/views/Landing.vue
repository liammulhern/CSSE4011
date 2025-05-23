<script setup lang="ts">
import { Card, CardHeader, CardTitle, CardContent } from '@/components/ui/card'
import { Button } from '@/components/ui/button'
import {
  Sun,
  MoonStar,
} from 'lucide-vue-next'
import { useRouter } from 'vue-router'
import { useAppStore } from '@/stores/app';

const router = useRouter();
const store = useAppStore();

function goLogin() {
  router.push({
    name: 'dashboard',
  });
}

function goGetStarted() {
  goLogin();
}

const toggleMode = () => {
  store.toggleTheme();
};
</script>

<template>
  <div class="min-h-screen flex flex-col bg-background text-foreground">
    <header class="flex items-center justify-between px-6 py-4">
      <h2 class="text-2xl text-primary font-semibold flex items-center">
        <span class="text-primary"><span class="mr-2 flex items-center">
            <Icon name="Package" />
          </span></span>
        PathLedger
      </h2>
      <div class="flex items-center justify-between">
        <Button variant="outline" class="border-0 p-[6px] ml-2 w-8 h-8" @click="toggleMode">
          <Sun v-if="store.isDark" />
          <MoonStar v-else />
        </Button>
        <div class="border-x-[1px] border-gray-300 h-[24px] w-[1px] mx-4 "></div>
        <Button @click="goLogin">
          Login
        </Button>
      </div>
    </header>

    <section class="flex-1 relative overflow-hidden">
      <div class="absolute inset-0 bg-cover bg-center filter brightness-50 saturate-0"
        style="background-image: url('/assets/images/landingpage-bg.png')"></div>
      <div class="absolute inset-0 bg-accent opacity-30"></div>

      <div class="absolute inset-0 z-10 flex flex-col items-center justify-center text-center px-6">
        <h2 class="text-5xl font-extrabold mb-4 text-primary-foreground drop-shadow-lg">
          Secure your supply chain with immutable ledgers
        </h2>
        <p class="max-w-lg text-lg text-primary-foreground mb-8">
          Trace every product’s journey end-to-end using IoT-backed blockchain proofs.
        </p>
        <Button class="px-8 py-3 text-primary-foreground bg-primary relative z-10" @click="goGetStarted">
          Get Started
        </Button>
      </div>

    </section>

    <section class="bg-muted py-16">
      <div class="max-w-5xl mx-auto grid grid-cols-1 md:grid-cols-3 gap-6 px-6">
        <Card>
          <CardHeader>
            <CardTitle class="flex items-center">
              Immutable Records
            </CardTitle>
          </CardHeader>
          <CardContent class="text-muted-foreground">
            Every movement is cryptographically signed and stored on-chain.
          </CardContent>
        </Card>

        <Card>
          <CardHeader>
            <CardTitle class="flex items-center">
              Real-Time Visibility
            </CardTitle>
          </CardHeader>
          <CardContent class="text-muted-foreground">
            Monitor status, location and condition of assets live.
          </CardContent>
        </Card>

        <Card>
          <CardHeader>
            <CardTitle class="flex items-center">
              Easy Integration
            </CardTitle>
          </CardHeader>
          <CardContent class="text-muted-foreground">
            Plug into existing sensors, gateways and blockchain nodes.
          </CardContent>
        </Card>
      </div>
    </section>

    <footer class="py-6 text-center text-muted-foreground">
      © {{ new Date().getFullYear() }} PathLedger — All rights reserved.
    </footer>
  </div>
</template>
